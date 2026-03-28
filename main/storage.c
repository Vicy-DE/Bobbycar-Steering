/**
 * @file storage.c
 * @brief LittleFS-based persistent storage for gamepad configs.
 *
 * Mounts a LittleFS partition and provides read/write access for
 * known-gamepad records and key=value application settings.
 */

#include "storage.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "esp_log.h"
#include "esp_littlefs.h"

static const char *TAG = "storage";

static const char *MOUNT_POINT   = "/littlefs";
static const char *GAMEPADS_FILE = "/littlefs/gamepads.bin";
static const char *CONFIG_DIR    = "/littlefs/config";

/* ------------------------------------------------------------------ */
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Validate a config key against path-traversal attacks.
 *
 * Only alphanumeric characters, underscores, and hyphens are allowed.
 *
 * @param[in] key  Key string to validate.
 * @return true if the key is safe.
 */
static bool key_is_valid(const char *key)
{
    if (key == NULL || key[0] == '\0') {
        return false;
    }
    for (const char *p = key; *p; ++p) {
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-') {
            return false;
        }
    }
    return true;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Mount the LittleFS partition and create directories.
 *
 * @return ESP_OK on success.
 *
 * @sideeffects Registers a VFS mount at /littlefs, formats on first use.
 */
esp_err_t storage_init(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path              = MOUNT_POINT,
        .partition_label        = "littlefs",
        .format_if_mount_failed = true,
        .dont_mount             = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LittleFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    esp_littlefs_info("littlefs", &total, &used);
    ESP_LOGI(TAG, "LittleFS mounted: total=%u used=%u",
             (unsigned)total, (unsigned)used);

    /* Create config directory if it does not exist. */
    struct stat st;
    if (stat(CONFIG_DIR, &st) != 0) {
        mkdir(CONFIG_DIR, 0755);
    }

    return ESP_OK;
}

/**
 * @brief Save or update a gamepad in the on-flash database.
 *
 * @param[in] addr  6-byte Bluetooth device address.
 * @param[in] name  Device name (may be NULL).
 * @return ESP_OK on success.
 *
 * @sideeffects Writes to LittleFS.
 */
esp_err_t storage_save_gamepad(const uint8_t addr[6], const char *name)
{
    storage_gamepad_entry_t entries[STORAGE_MAX_GAMEPADS];
    int count = storage_load_gamepads(entries, STORAGE_MAX_GAMEPADS);

    /* Update existing entry if address already known. */
    for (int i = 0; i < count; i++) {
        if (memcmp(entries[i].addr, addr, 6) == 0) {
            if (name) {
                strncpy(entries[i].name, name, STORAGE_NAME_LEN - 1);
                entries[i].name[STORAGE_NAME_LEN - 1] = '\0';
            }
            goto write_file;
        }
    }

    /* Evict oldest entry when database is full. */
    if (count >= STORAGE_MAX_GAMEPADS) {
        memmove(&entries[0], &entries[1],
                sizeof(entries[0]) * (STORAGE_MAX_GAMEPADS - 1));
        count = STORAGE_MAX_GAMEPADS - 1;
    }

    memcpy(entries[count].addr, addr, 6);
    if (name) {
        strncpy(entries[count].name, name, STORAGE_NAME_LEN - 1);
        entries[count].name[STORAGE_NAME_LEN - 1] = '\0';
    } else {
        entries[count].name[0] = '\0';
    }
    count++;

write_file: ;
    FILE *f = fopen(GAMEPADS_FILE, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for writing", GAMEPADS_FILE);
        return ESP_FAIL;
    }
    fwrite(&count, sizeof(count), 1, f);
    fwrite(entries, sizeof(entries[0]), count, f);
    fclose(f);

    ESP_LOGI(TAG, "Saved gamepad %02X:%02X:%02X:%02X:%02X:%02X (%s), "
             "total=%d",
             addr[0], addr[1], addr[2], addr[3], addr[4], addr[5],
             name ? name : "(unknown)", count);
    return ESP_OK;
}

/**
 * @brief Remove a gamepad from the on-flash database.
 *
 * @param[in] addr  6-byte Bluetooth device address.
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if absent.
 *
 * @sideeffects Writes to LittleFS.
 */
esp_err_t storage_remove_gamepad(const uint8_t addr[6])
{
    storage_gamepad_entry_t entries[STORAGE_MAX_GAMEPADS];
    int count = storage_load_gamepads(entries, STORAGE_MAX_GAMEPADS);

    int idx = -1;
    for (int i = 0; i < count; i++) {
        if (memcmp(entries[i].addr, addr, 6) == 0) {
            idx = i;
            break;
        }
    }

    if (idx < 0) {
        return ESP_ERR_NOT_FOUND;
    }

    memmove(&entries[idx], &entries[idx + 1],
            sizeof(entries[0]) * (count - idx - 1));
    count--;

    FILE *f = fopen(GAMEPADS_FILE, "wb");
    if (!f) {
        return ESP_FAIL;
    }
    fwrite(&count, sizeof(count), 1, f);
    fwrite(entries, sizeof(entries[0]), count, f);
    fclose(f);
    return ESP_OK;
}

/**
 * @brief Load all known gamepads from persistent storage.
 *
 * @param[out] out          Array to receive entries.
 * @param[in]  max_entries  Capacity of @p out.
 * @return Number of entries written (0 if file absent or empty).
 */
int storage_load_gamepads(storage_gamepad_entry_t *out, int max_entries)
{
    FILE *f = fopen(GAMEPADS_FILE, "rb");
    if (!f) {
        return 0;
    }

    int count = 0;
    if (fread(&count, sizeof(count), 1, f) != 1) {
        fclose(f);
        return 0;
    }

    if (count > max_entries) {
        count = max_entries;
    }
    if (count > STORAGE_MAX_GAMEPADS) {
        count = STORAGE_MAX_GAMEPADS;
    }

    int n_read = (int)fread(out, sizeof(out[0]), count, f);
    fclose(f);
    return n_read;
}

/**
 * @brief Save a key=value string to a config file.
 *
 * @param[in] key    Config key (alphanumeric/underscore/hyphen only).
 * @param[in] value  NUL-terminated value string.
 * @return ESP_OK on success.
 *
 * @sideeffects Writes to LittleFS.
 */
esp_err_t storage_save_config(const char *key, const char *value)
{
    if (!key_is_valid(key)) {
        ESP_LOGE(TAG, "Invalid config key: %s", key ? key : "(null)");
        return ESP_ERR_INVALID_ARG;
    }

    char path[128];
    snprintf(path, sizeof(path), "%s/%s", CONFIG_DIR, key);

    FILE *f = fopen(path, "w");
    if (!f) {
        return ESP_FAIL;
    }
    fputs(value, f);
    fclose(f);
    return ESP_OK;
}

/**
 * @brief Load a key=value string from a config file.
 *
 * @param[in]  key      Config key.
 * @param[out] value    Buffer to receive the value.
 * @param[in]  max_len  Size of @p value buffer.
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if absent.
 */
esp_err_t storage_load_config(const char *key, char *value, size_t max_len)
{
    if (!key_is_valid(key)) {
        return ESP_ERR_INVALID_ARG;
    }

    char path[128];
    snprintf(path, sizeof(path), "%s/%s", CONFIG_DIR, key);

    FILE *f = fopen(path, "r");
    if (!f) {
        return ESP_ERR_NOT_FOUND;
    }

    if (fgets(value, (int)max_len, f) == NULL) {
        fclose(f);
        return ESP_FAIL;
    }
    fclose(f);
    return ESP_OK;
}
