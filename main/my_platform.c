/**
 * @file my_platform.c
 * @brief Custom Bluepad32 platform for Bobbycar-Steering.
 *
 * Implements the Bluepad32 platform callbacks for BLE gamepad
 * support.  Logs connection events and controller data (axes,
 * buttons) to the serial console.  Persists known gamepads
 * and configs to LittleFS.
 */

#include <string.h>
#include <inttypes.h>

#include <uni.h>

#include "esp_log.h"
#include "storage.h"
#include "bp32_config.h"
#include "ble_console.h"

static const char *TAG = "gamepad";

/* Custom per-device instance data. */
typedef struct my_platform_instance_s {
    uni_gamepad_seat_t gamepad_seat;
} my_platform_instance_t;

/* Forward declarations */
static my_platform_instance_t *get_my_platform_instance(uni_hid_device_t *d);
static void trigger_event_on_gamepad(uni_hid_device_t *d);

/* ------------------------------------------------------------------ */
/*  Platform callbacks                                                */
/* ------------------------------------------------------------------ */

/**
 * @brief Initialize the custom platform.
 *
 * @param[in] argc  Unused argument count.
 * @param[in] argv  Unused argument vector.
 *
 * @sideeffects None.
 */
static void my_platform_init(int argc, const char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    ESP_LOGI(TAG, "Bluepad32 custom platform initialized");
}

/**
 * @brief Called when Bluepad32 initialization is complete.
 *
 * Loads known gamepads from LittleFS, starts BLE scanning,
 * and enables incoming gamepad connections.
 *
 * @sideeffects Starts Bluetooth scanning via BTstack,
 *              reads known gamepads from LittleFS.
 */
static void my_platform_on_init_complete(void)
{
    ESP_LOGI(TAG, "Bluepad32 init complete \xe2\x80\x94 scanning for gamepads");

    /* Load Bluepad32 config (.ini files) from LittleFS. */
    bp32_config_load();

    /* Initialise BLE NUS console (coexists with Bluepad32). */
    ble_console_init();

    /* Load known gamepads from LittleFS and log them. */
    storage_gamepad_entry_t known[STORAGE_MAX_GAMEPADS];
    int count = storage_load_gamepads(known, STORAGE_MAX_GAMEPADS);
    ESP_LOGI(TAG, "Loaded %d known gamepad(s) from storage", count);
    for (int i = 0; i < count; i++) {
        ESP_LOGI(TAG, "  [%d] %02X:%02X:%02X:%02X:%02X:%02X  %s",
                 i,
                 known[i].addr[0], known[i].addr[1], known[i].addr[2],
                 known[i].addr[3], known[i].addr[4], known[i].addr[5],
                 known[i].name);
    }

    uni_bt_start_scanning_and_autoconnect_unsafe();
    uni_bt_allow_incoming_connections(true);
}

/**
 * @brief Filter discovered Bluetooth devices.
 *
 * @param[in] addr  Bluetooth address of the discovered device.
 * @param[in] name  Device name (may be NULL).
 * @param[in] cod   Class of Device.
 * @param[in] rssi  Received Signal Strength Indicator.
 * @return UNI_ERROR_SUCCESS to accept, UNI_ERROR_IGNORE_DEVICE to reject.
 */
static uni_error_t my_platform_on_device_discovered(bd_addr_t addr,
                                                     const char *name,
                                                     uint16_t cod,
                                                     uint8_t rssi)
{
    /* Filter out keyboards. */
    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) ==
        UNI_BT_COD_MINOR_KEYBOARD) {
        ESP_LOGI(TAG, "Ignoring keyboard device");
        return UNI_ERROR_IGNORE_DEVICE;
    }

    return UNI_ERROR_SUCCESS;
}

/**
 * @brief Called when a device connects.
 *
 * @param[in] d  Pointer to the HID device.
 *
 * @sideeffects Logs connection event.
 */
static void my_platform_on_device_connected(uni_hid_device_t *d)
{
    ESP_LOGI(TAG, "Device connected: %p", d);
}

/**
 * @brief Called when a device disconnects.
 *
 * @param[in] d  Pointer to the HID device.
 *
 * @sideeffects Logs disconnection event.
 */
static void my_platform_on_device_disconnected(uni_hid_device_t *d)
{
    ESP_LOGI(TAG, "Device disconnected: %p", d);
}

/**
 * @brief Called when a device is ready to use.
 *
 * Assigns the gamepad to seat A, triggers a connection event
 * (rumble + LED feedback if supported), and persists the
 * gamepad in LittleFS storage.
 *
 * @param[in] d  Pointer to the HID device.
 * @return UNI_ERROR_SUCCESS on success.
 *
 * @sideeffects Sets gamepad seat, may trigger rumble and LED state,
 *              writes to LittleFS.
 */
static uni_error_t my_platform_on_device_ready(uni_hid_device_t *d)
{
    ESP_LOGI(TAG, "Device ready: %p", d);
    my_platform_instance_t *ins = get_my_platform_instance(d);
    ins->gamepad_seat = GAMEPAD_SEAT_A;

    /* Persist this gamepad in LittleFS. */
    storage_save_gamepad(d->conn.btaddr, d->name);

    /* Add to Bluepad32 known device config. */
    bp32_config_add_device(d->conn.btaddr, d->name);

    trigger_event_on_gamepad(d);
    return UNI_ERROR_SUCCESS;
}

/**
 * @brief Called on every controller data update.
 *
 * Logs gamepad axes and button state to the serial console.
 * Only logs when data changes to avoid spamming.
 *
 * @param[in] d    Pointer to the HID device.
 * @param[in] ctl  Pointer to the controller data.
 *
 * @sideeffects Writes log output to serial console.
 */
static void my_platform_on_controller_data(uni_hid_device_t *d,
                                            uni_controller_t *ctl)
{
    static uni_controller_t prev = {0};

    /* Only process when data changes. */
    if (memcmp(&prev, ctl, sizeof(*ctl)) == 0) {
        return;
    }
    prev = *ctl;

    if (ctl->klass != UNI_CONTROLLER_CLASS_GAMEPAD) {
        return;
    }

    uni_gamepad_t *gp = &ctl->gamepad;

    ESP_LOGI(TAG, "Gamepad: LX=%4" PRId32 " LY=%4" PRId32
             " RX=%4" PRId32 " RY=%4" PRId32
             " btns=0x%04" PRIx32 " misc=0x%02" PRIx32
             " brake=%4" PRId32 " throttle=%4" PRId32,
             gp->axis_x, gp->axis_y,
             gp->axis_rx, gp->axis_ry,
             (uint32_t)gp->buttons, (uint32_t)gp->misc_buttons,
             gp->brake, gp->throttle);
}

/**
 * @brief Query platform properties.
 *
 * @param[in] idx  Property index.
 * @return NULL (no custom properties).
 */
static const uni_property_t *my_platform_get_property(uni_property_idx_t idx)
{
    ARG_UNUSED(idx);
    return NULL;
}

/**
 * @brief Handle out-of-band events (system button, BT enable, etc.).
 *
 * @param[in] event  The OOB event type.
 * @param[in] data   Event-specific data.
 *
 * @sideeffects May trigger rumble or LED changes on gamepad.
 */
static void my_platform_on_oob_event(uni_platform_oob_event_t event,
                                      void *data)
{
    switch (event) {
    case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON: {
        uni_hid_device_t *d = data;
        if (d == NULL) {
            return;
        }
        ESP_LOGI(TAG, "System button pressed");
        my_platform_instance_t *ins = get_my_platform_instance(d);
        ins->gamepad_seat = (ins->gamepad_seat == GAMEPAD_SEAT_A)
                                ? GAMEPAD_SEAT_B
                                : GAMEPAD_SEAT_A;
        trigger_event_on_gamepad(d);
        break;
    }
    case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
        ESP_LOGI(TAG, "Bluetooth enabled: %d", (bool)(data));
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Get the custom platform instance for a device.
 *
 * @param[in] d  Pointer to the HID device.
 * @return Pointer to the platform instance data.
 */
static my_platform_instance_t *get_my_platform_instance(uni_hid_device_t *d)
{
    return (my_platform_instance_t *)&d->platform_data[0];
}

/**
 * @brief Trigger a connection event on the gamepad.
 *
 * Sends a brief rumble and sets LEDs/lightbar if supported.
 *
 * @param[in] d  Pointer to the HID device.
 *
 * @sideeffects Writes to gamepad via BT (rumble, LEDs).
 */
static void trigger_event_on_gamepad(uni_hid_device_t *d)
{
    my_platform_instance_t *ins = get_my_platform_instance(d);

    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0, 150, 128, 40);
    }

    if (d->report_parser.set_player_leds != NULL) {
        d->report_parser.set_player_leds(d, ins->gamepad_seat);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        uint8_t r = (ins->gamepad_seat & 0x01) ? 0xff : 0;
        uint8_t g = (ins->gamepad_seat & 0x02) ? 0xff : 0;
        uint8_t b = (ins->gamepad_seat & 0x04) ? 0xff : 0;
        d->report_parser.set_lightbar_color(d, r, g, b);
    }
}

/* ------------------------------------------------------------------ */
/*  Entry point — called from main.c                                  */
/* ------------------------------------------------------------------ */

/**
 * @brief Return the custom Bluepad32 platform struct.
 *
 * @return Pointer to the static uni_platform struct.
 */
struct uni_platform *get_my_platform(void)
{
    static struct uni_platform plat = {
        .name = "bobbycar",
        .init = my_platform_init,
        .on_init_complete = my_platform_on_init_complete,
        .on_device_discovered = my_platform_on_device_discovered,
        .on_device_connected = my_platform_on_device_connected,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_device_ready = my_platform_on_device_ready,
        .on_oob_event = my_platform_on_oob_event,
        .on_controller_data = my_platform_on_controller_data,
        .get_property = my_platform_get_property,
    };

    return &plat;
}
