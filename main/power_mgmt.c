/**
 * @file power_mgmt.c
 * @brief ESP32 power management implementation.
 *
 * Uses esp_pm_configure() to enable dynamic frequency scaling
 * and automatic light sleep.  On RISC-V ESP32 chips the CPU
 * executes WFI during FreeRTOS idle ticks, reducing power to
 * a fraction of active mode.
 *
 * Prerequisites in sdkconfig:
 *   CONFIG_PM_ENABLE=y
 *   CONFIG_FREERTOS_USE_TICKLESS_IDLE=y (optional, for light
 *   sleep)
 */

#include "power_mgmt.h"

#include "esp_pm.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"

static const char *TAG = "power";

void power_mgmt_init(void)
{
#if CONFIG_PM_ENABLE
    esp_pm_config_t cfg = {
#if CONFIG_IDF_TARGET_ESP32C3
        .max_freq_mhz = 160,
        .min_freq_mhz = 80,
#elif CONFIG_IDF_TARGET_ESP32H2
        .max_freq_mhz = 96,
        .min_freq_mhz = 32,
#elif CONFIG_IDF_TARGET_ESP32C5
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
#else
        .max_freq_mhz = 160,
        .min_freq_mhz = 80,
#endif
        .light_sleep_enable = false,
    };

    esp_err_t ret = esp_pm_configure(&cfg);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Power management enabled"
                 " (DFS %d-%d MHz)",
                 cfg.min_freq_mhz, cfg.max_freq_mhz);
    } else {
        ESP_LOGW(TAG, "PM config failed: %s"
                 " (continuing without DFS)",
                 esp_err_to_name(ret));
    }
#else
    ESP_LOGW(TAG, "CONFIG_PM_ENABLE not set"
             " — power management disabled");
#endif
}
