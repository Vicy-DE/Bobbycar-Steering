/**
 * @file bp32_config.c
 * @brief Bluepad32 INI config implementation.
 *
 * Manages general settings, known device allowlist, and
 * button mapping via INI files on LittleFS.
 */

#include "bp32_config.h"
#include "ini_parser.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include "esp_log.h"

static const char *TAG = "bp32_cfg";

/* -------------------------------------------------------- */
/*  File paths                                              */
/* -------------------------------------------------------- */

#define CFG_DIR       "/littlefs/config"
#define GENERAL_INI   CFG_DIR "/bluepad32.ini"
#define BUTTONMAP_INI CFG_DIR "/buttonmap.ini"
#define DEVICES_DIR   CFG_DIR "/devices"

/* -------------------------------------------------------- */
/*  State                                                   */
/* -------------------------------------------------------- */

static bp32_general_config_t s_general = {
    .max_devices      = 4,
    .scan_timeout_s   = 0,
    .filter_keyboards = true,
    .auto_reconnect   = true,
};

static bp32_button_map_t s_buttonmap = {
    .throttle_axis    = 1,   /* Left stick Y */
    .steering_axis    = 0,   /* Left stick X */
    .boost_button     = 0x0001,  /* A / Cross */
    .brake_button     = 0x0002,  /* B / Circle */
    .horn_button      = 0x0008,  /* Y / Triangle */
    .lights_button    = 0x0004,  /* X / Square */
    .invert_throttle  = false,
    .invert_steering  = false,
};

static bp32_known_device_t s_devices[BP32_MAX_KNOWN_DEVICES];
static int s_device_count = 0;

/* -------------------------------------------------------- */
/*  INI callbacks                                           */
/* -------------------------------------------------------- */

/**
 * @brief Handler for general config INI entries.
 */
static bool general_handler(const char *section,
                            const char *key,
                            const char *value,
                            void *user)
{
    (void)user;
    if (strcmp(section, "general") != 0) {
        return true;
    }
    if (strcmp(key, "max_devices") == 0) {
        s_general.max_devices = atoi(value);
    } else if (strcmp(key, "scan_timeout") == 0) {
        s_general.scan_timeout_s = atoi(value);
    } else if (strcmp(key, "filter_keyboards") == 0) {
        s_general.filter_keyboards =
            (strcmp(value, "1") == 0
             || strcmp(value, "true") == 0);
    } else if (strcmp(key, "auto_reconnect") == 0) {
        s_general.auto_reconnect =
            (strcmp(value, "1") == 0
             || strcmp(value, "true") == 0);
    }
    return true;
}

/**
 * @brief Handler for button mapping INI entries.
 */
static bool buttonmap_handler(const char *section,
                              const char *key,
                              const char *value,
                              void *user)
{
    (void)user;
    if (strcmp(section, "axes") == 0) {
        if (strcmp(key, "throttle") == 0) {
            s_buttonmap.throttle_axis = atoi(value);
        } else if (strcmp(key, "steering") == 0) {
            s_buttonmap.steering_axis = atoi(value);
        } else if (strcmp(key, "invert_throttle") == 0) {
            s_buttonmap.invert_throttle =
                (strcmp(value, "1") == 0
                 || strcmp(value, "true") == 0);
        } else if (strcmp(key, "invert_steering") == 0) {
            s_buttonmap.invert_steering =
                (strcmp(value, "1") == 0
                 || strcmp(value, "true") == 0);
        }
    } else if (strcmp(section, "buttons") == 0) {
        int val = (int)strtol(value, NULL, 0);
        if (strcmp(key, "boost") == 0) {
            s_buttonmap.boost_button = val;
        } else if (strcmp(key, "brake") == 0) {
            s_buttonmap.brake_button = val;
        } else if (strcmp(key, "horn") == 0) {
            s_buttonmap.horn_button = val;
        } else if (strcmp(key, "lights") == 0) {
            s_buttonmap.lights_button = val;
        }
    }
    return true;
}

/**
 * @brief Handler for a single device INI file.
 */
static bool device_handler(const char *section,
                           const char *key,
                           const char *value,
                           void *user)
{
    bp32_known_device_t *dev = (bp32_known_device_t *)user;
    if (strcmp(section, "device") != 0) {
        return true;
    }
    if (strcmp(key, "name") == 0) {
        strncpy(dev->name, value, sizeof(dev->name) - 1);
        dev->name[sizeof(dev->name) - 1] = '\0';
    } else if (strcmp(key, "autoconnect") == 0) {
        dev->autoconnect =
            (strcmp(value, "1") == 0
             || strcmp(value, "true") == 0);
    } else if (strcmp(key, "addr") == 0) {
        /* Parse XX:XX:XX:XX:XX:XX */
        unsigned int a[6];
        if (sscanf(value,
                   "%02x:%02x:%02x:%02x:%02x:%02x",
                   &a[0], &a[1], &a[2],
                   &a[3], &a[4], &a[5]) == 6) {
            for (int i = 0; i < 6; i++) {
                dev->addr[i] = (uint8_t)a[i];
            }
        }
    }
    return true;
}

/* -------------------------------------------------------- */
/*  Create defaults                                         */
/* -------------------------------------------------------- */

/**
 * @brief Create default bluepad32.ini if missing.
 *
 * @sideeffects Writes to LittleFS.
 */
static void create_default_general(void)
{
    struct stat st;
    if (stat(GENERAL_INI, &st) == 0) {
        return;
    }
    FILE *f = fopen(GENERAL_INI, "w");
    if (!f) {
        return;
    }
    fprintf(f,
        "# Bluepad32 General Configuration\n"
        "[general]\n"
        "max_devices=4\n"
        "scan_timeout=0\n"
        "filter_keyboards=true\n"
        "auto_reconnect=true\n");
    fclose(f);
    ESP_LOGI(TAG, "Created default %s", GENERAL_INI);
}

/**
 * @brief Create default buttonmap.ini if missing.
 *
 * @sideeffects Writes to LittleFS.
 */
static void create_default_buttonmap(void)
{
    struct stat st;
    if (stat(BUTTONMAP_INI, &st) == 0) {
        return;
    }
    FILE *f = fopen(BUTTONMAP_INI, "w");
    if (!f) {
        return;
    }
    fprintf(f,
        "# Button Mapping Configuration\n"
        "# Axis indices: 0=LX, 1=LY, 2=RX, 3=RY\n"
        "# Button values: bitmask (hex with 0x prefix)\n"
        "[axes]\n"
        "throttle=1\n"
        "steering=0\n"
        "invert_throttle=false\n"
        "invert_steering=false\n"
        "\n"
        "[buttons]\n"
        "boost=0x0001\n"
        "brake=0x0002\n"
        "horn=0x0008\n"
        "lights=0x0004\n");
    fclose(f);
    ESP_LOGI(TAG, "Created default %s", BUTTONMAP_INI);
}

/**
 * @brief Ensure devices directory exists.
 *
 * @sideeffects Creates directory on LittleFS.
 */
static void ensure_devices_dir(void)
{
    struct stat st;
    if (stat(DEVICES_DIR, &st) != 0) {
        mkdir(DEVICES_DIR, 0755);
    }
}

/* -------------------------------------------------------- */
/*  Load devices from /config/devices/ (.ini files)         */
/* -------------------------------------------------------- */

/**
 * @brief Scan devices directory and load all .ini files.
 *
 * @sideeffects Reads from LittleFS.
 */
static void load_devices(void)
{
    s_device_count = 0;

    DIR *d = opendir(DEVICES_DIR);
    if (!d) {
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL
           && s_device_count < BP32_MAX_KNOWN_DEVICES) {
        /* Only process .ini files. */
        const char *dot = strrchr(ent->d_name, '.');
        if (!dot || strcmp(dot, ".ini") != 0) {
            continue;
        }
        /* Skip names that would overflow the path buffer. */
        if (strlen(ent->d_name) > 32) {
            continue;
        }

        char path[64];
        snprintf(path, sizeof(path),
                 "%s/%.32s", DEVICES_DIR, ent->d_name);

        bp32_known_device_t *dev =
            &s_devices[s_device_count];
        memset(dev, 0, sizeof(*dev));
        dev->autoconnect = true;

        if (ini_parse_file(path, device_handler,
                           dev) >= 0) {
            s_device_count++;
            ESP_LOGI(TAG, "Loaded device: "
                     "%02X:%02X:%02X:%02X:%02X:%02X "
                     "(%s, auto=%d)",
                     dev->addr[0], dev->addr[1],
                     dev->addr[2], dev->addr[3],
                     dev->addr[4], dev->addr[5],
                     dev->name, dev->autoconnect);
        }
    }
    closedir(d);
}

/* -------------------------------------------------------- */
/*  Public API                                              */
/* -------------------------------------------------------- */

/* See bp32_config.h for doc comment. */
void bp32_config_load(void)
{
    ensure_devices_dir();
    create_default_general();
    create_default_buttonmap();

    ini_parse_file(GENERAL_INI, general_handler, NULL);
    ini_parse_file(BUTTONMAP_INI, buttonmap_handler, NULL);
    load_devices();

    ESP_LOGI(TAG, "Config loaded: max_dev=%d, "
             "filter_kb=%d, auto_recon=%d, "
             "%d known device(s)",
             s_general.max_devices,
             s_general.filter_keyboards,
             s_general.auto_reconnect,
             s_device_count);
}

/* See bp32_config.h for doc comment. */
void bp32_config_save(void)
{
    /* General config. */
    ini_write_value(GENERAL_INI, "general",
                    "max_devices",
                    s_general.max_devices == 4 ? "4" : "1");
    ini_write_value(GENERAL_INI, "general",
                    "filter_keyboards",
                    s_general.filter_keyboards
                        ? "true" : "false");
    ini_write_value(GENERAL_INI, "general",
                    "auto_reconnect",
                    s_general.auto_reconnect
                        ? "true" : "false");

    /* Button map. */
    char buf[16];
    snprintf(buf, sizeof(buf), "%d",
             s_buttonmap.throttle_axis);
    ini_write_value(BUTTONMAP_INI, "axes",
                    "throttle", buf);
    snprintf(buf, sizeof(buf), "%d",
             s_buttonmap.steering_axis);
    ini_write_value(BUTTONMAP_INI, "axes",
                    "steering", buf);
    ini_write_value(BUTTONMAP_INI, "axes",
                    "invert_throttle",
                    s_buttonmap.invert_throttle
                        ? "true" : "false");
    ini_write_value(BUTTONMAP_INI, "axes",
                    "invert_steering",
                    s_buttonmap.invert_steering
                        ? "true" : "false");

    snprintf(buf, sizeof(buf), "0x%04x",
             s_buttonmap.boost_button);
    ini_write_value(BUTTONMAP_INI, "buttons",
                    "boost", buf);
    snprintf(buf, sizeof(buf), "0x%04x",
             s_buttonmap.brake_button);
    ini_write_value(BUTTONMAP_INI, "buttons",
                    "brake", buf);
    snprintf(buf, sizeof(buf), "0x%04x",
             s_buttonmap.horn_button);
    ini_write_value(BUTTONMAP_INI, "buttons",
                    "horn", buf);
    snprintf(buf, sizeof(buf), "0x%04x",
             s_buttonmap.lights_button);
    ini_write_value(BUTTONMAP_INI, "buttons",
                    "lights", buf);

    ESP_LOGI(TAG, "Config saved to LittleFS");
}

/* See bp32_config.h for doc comment. */
const bp32_general_config_t *bp32_config_get_general(void)
{
    return &s_general;
}

/* See bp32_config.h for doc comment. */
const bp32_button_map_t *bp32_config_get_buttonmap(void)
{
    return &s_buttonmap;
}

/* See bp32_config.h for doc comment. */
void bp32_config_add_device(const uint8_t addr[6],
                            const char *name)
{
    ensure_devices_dir();

    /* Check if already known. */
    for (int i = 0; i < s_device_count; i++) {
        if (memcmp(s_devices[i].addr, addr, 6) == 0) {
            if (name) {
                strncpy(s_devices[i].name, name,
                        sizeof(s_devices[i].name) - 1);
                s_devices[i].name[
                    sizeof(s_devices[i].name) - 1] = '\0';
            }
            /* Re-save the file. */
            goto write_file;
        }
    }

    /* Add new entry. */
    if (s_device_count >= BP32_MAX_KNOWN_DEVICES) {
        /* Evict oldest. */
        memmove(&s_devices[0], &s_devices[1],
                sizeof(s_devices[0])
                    * (BP32_MAX_KNOWN_DEVICES - 1));
        s_device_count = BP32_MAX_KNOWN_DEVICES - 1;
    }

    bp32_known_device_t *dev =
        &s_devices[s_device_count];
    memcpy(dev->addr, addr, 6);
    if (name) {
        strncpy(dev->name, name,
                sizeof(dev->name) - 1);
        dev->name[sizeof(dev->name) - 1] = '\0';
    } else {
        dev->name[0] = '\0';
    }
    dev->autoconnect = true;
    s_device_count++;

write_file: ;
    /* Write device INI file named by address. */
    char filename[128];
    snprintf(filename, sizeof(filename),
             "%s/%02x%02x%02x%02x%02x%02x.ini",
             DEVICES_DIR,
             addr[0], addr[1], addr[2],
             addr[3], addr[4], addr[5]);

    /* Find the device we just added/updated. */
    bp32_known_device_t *target = NULL;
    for (int i = 0; i < s_device_count; i++) {
        if (memcmp(s_devices[i].addr, addr, 6) == 0) {
            target = &s_devices[i];
            break;
        }
    }
    if (!target) {
        return;
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
        ESP_LOGE(TAG, "Cannot write %s", filename);
        return;
    }
    fprintf(f,
        "# Known Device\n"
        "[device]\n"
        "addr=%02X:%02X:%02X:%02X:%02X:%02X\n"
        "name=%s\n"
        "autoconnect=%s\n",
        target->addr[0], target->addr[1],
        target->addr[2], target->addr[3],
        target->addr[4], target->addr[5],
        target->name,
        target->autoconnect ? "true" : "false");
    fclose(f);
}

/* See bp32_config.h for doc comment. */
int bp32_config_get_devices(bp32_known_device_t *out,
                            int max)
{
    int n = s_device_count;
    if (n > max) {
        n = max;
    }
    memcpy(out, s_devices,
           sizeof(s_devices[0]) * (size_t)n);
    return n;
}

/* See bp32_config.h for doc comment. */
bool bp32_config_is_known(const uint8_t addr[6])
{
    for (int i = 0; i < s_device_count; i++) {
        if (memcmp(s_devices[i].addr, addr, 6) == 0
            && s_devices[i].autoconnect) {
            return true;
        }
    }
    return false;
}
