/**
 * @file storage.h
 * @brief LittleFS-based persistent storage for gamepad configs.
 *
 * Provides functions to mount a LittleFS partition and persist
 * known-gamepad records and key=value application config.
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

/** Maximum gamepads tracked in the on-flash database. */
#define STORAGE_MAX_GAMEPADS  8

/** Maximum device-name length (including NUL). */
#define STORAGE_NAME_LEN      64

/** One known-gamepad record. */
typedef struct {
    uint8_t  addr[6];                   /**< Bluetooth device address. */
    char     name[STORAGE_NAME_LEN];    /**< Human-readable name.      */
} storage_gamepad_entry_t;

/**
 * @brief Mount the LittleFS partition and create directories.
 *
 * @return ESP_OK on success.
 *
 * @sideeffects Registers a VFS mount at /littlefs, formats on first use.
 */
esp_err_t storage_init(void);

/**
 * @brief Save or update a gamepad in the on-flash database.
 *
 * If the address already exists the name is updated in place.
 * When the database is full the oldest entry is evicted.
 *
 * @param[in] addr  6-byte Bluetooth device address.
 * @param[in] name  Device name (may be NULL).
 * @return ESP_OK on success.
 *
 * @sideeffects Writes to LittleFS.
 */
esp_err_t storage_save_gamepad(const uint8_t addr[6], const char *name);

/**
 * @brief Remove a gamepad from the on-flash database.
 *
 * @param[in] addr  6-byte Bluetooth device address.
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if absent.
 *
 * @sideeffects Writes to LittleFS.
 */
esp_err_t storage_remove_gamepad(const uint8_t addr[6]);

/**
 * @brief Load all known gamepads from persistent storage.
 *
 * @param[out] out          Array to receive entries.
 * @param[in]  max_entries  Capacity of @p out.
 * @return Number of entries written (0 if file absent or empty).
 */
int storage_load_gamepads(storage_gamepad_entry_t *out, int max_entries);

/**
 * @brief Save a key=value string to a config file.
 *
 * @param[in] key    Config key (alphanumeric/underscore, no path separators).
 * @param[in] value  NUL-terminated value string.
 * @return ESP_OK on success.
 *
 * @sideeffects Writes to LittleFS.
 */
esp_err_t storage_save_config(const char *key, const char *value);

/**
 * @brief Load a key=value string from a config file.
 *
 * @param[in]  key      Config key.
 * @param[out] value    Buffer to receive the value.
 * @param[in]  max_len  Size of @p value buffer.
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if absent.
 */
esp_err_t storage_load_config(const char *key, char *value, size_t max_len);
