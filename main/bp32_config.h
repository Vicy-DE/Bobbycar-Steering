/**
 * @file bp32_config.h
 * @brief Bluepad32 configuration via INI files on LittleFS.
 *
 * Manages Bluepad32 settings, known device allowlist, and
 * button mapping using INI files stored on the filesystem.
 *
 * Files:
 *   /littlefs/config/bluepad32.ini   — General settings
 *   /littlefs/config/devices/        — Per-device INI files
 *   /littlefs/config/buttonmap.ini   — Button-to-action map
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------- */
/*  General config (bluepad32.ini)                          */
/* -------------------------------------------------------- */

/** Maximum number of known devices in allowlist. */
#define BP32_MAX_KNOWN_DEVICES  8

/**
 * @brief General Bluepad32 configuration.
 */
typedef struct {
    int  max_devices;       /**< Max concurrent connections.  */
    int  scan_timeout_s;    /**< BLE scan timeout (seconds).  */
    bool filter_keyboards;  /**< Reject keyboard HID devices. */
    bool auto_reconnect;    /**< Auto-reconnect known devices.*/
} bp32_general_config_t;

/* -------------------------------------------------------- */
/*  Known device (per-device .ini in devices/)              */
/* -------------------------------------------------------- */

/**
 * @brief Known device entry for autoconnect allowlist.
 */
typedef struct {
    uint8_t addr[6];       /**< BT address.                  */
    char    name[32];      /**< Friendly name.               */
    bool    autoconnect;   /**< Allow autoconnect.           */
} bp32_known_device_t;

/* -------------------------------------------------------- */
/*  Button mapping (buttonmap.ini)                          */
/* -------------------------------------------------------- */

/**
 * @brief Button mapping: maps gamepad inputs to actions.
 */
typedef struct {
    int throttle_axis;    /**< Axis index for throttle.      */
    int steering_axis;    /**< Axis index for steering.      */
    int boost_button;     /**< Button bit for boost.         */
    int brake_button;     /**< Button bit for brake.         */
    int horn_button;      /**< Button bit for horn.          */
    int lights_button;    /**< Button bit for lights toggle. */
    bool invert_throttle; /**< Invert throttle axis.         */
    bool invert_steering; /**< Invert steering axis.         */
} bp32_button_map_t;

/* -------------------------------------------------------- */
/*  API                                                     */
/* -------------------------------------------------------- */

/**
 * @brief Load all Bluepad32 config from LittleFS.
 *
 * Reads bluepad32.ini, device .ini files, and buttonmap.ini.
 * Creates default files if they do not exist.
 *
 * @sideeffects Reads/writes LittleFS files.
 */
void bp32_config_load(void);

/**
 * @brief Save current config back to LittleFS.
 *
 * @sideeffects Writes to LittleFS files.
 */
void bp32_config_save(void);

/**
 * @brief Get the general config (read-only).
 *
 * @return Pointer to the current general config.
 */
const bp32_general_config_t *bp32_config_get_general(void);

/**
 * @brief Get the button mapping (read-only).
 *
 * @return Pointer to the current button map.
 */
const bp32_button_map_t *bp32_config_get_buttonmap(void);

/**
 * @brief Add or update a known device.
 *
 * @param[in] addr  6-byte BT address.
 * @param[in] name  Device name (may be NULL).
 *
 * @sideeffects Writes INI file to LittleFS.
 */
void bp32_config_add_device(const uint8_t addr[6],
                            const char *name);

/**
 * @brief Get list of known devices.
 *
 * @param[out] out   Array to receive entries.
 * @param[in]  max   Capacity of @p out.
 * @return Number of entries.
 */
int bp32_config_get_devices(bp32_known_device_t *out,
                            int max);

/**
 * @brief Check if a device is in the known allowlist.
 *
 * @param[in] addr  6-byte BT address.
 * @return true if the device is known and autoconnect enabled.
 */
bool bp32_config_is_known(const uint8_t addr[6]);

#ifdef __cplusplus
}
#endif
