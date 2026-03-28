/**
 * @file main.c
 * @brief Bobbycar-Steering application entry point.
 *
 * Cross-platform firmware for ESP32-H2, ESP32-C3, and ESP32-C5
 * SuperMini development boards. Reads two ADC analog sensor
 * channels and displays the values on a 3.5" ST7796S SPI TFT
 * using LVGL.  Supports BLE gamepad input via Bluepad32.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "lvgl.h"
#include "display.h"
#include "pin_config.h"
#include "storage.h"

#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <uni.h>

#include "sdkconfig.h"

#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Kconfig: must set CONFIG_BLUEPAD32_PLATFORM_CUSTOM=y"
#endif

/* Defined in my_platform.c */
struct uni_platform *get_my_platform(void);

static const char *TAG = "main";

/* ADC handle */
static adc_oneshot_unit_handle_t s_adc_handle = NULL;

/* LVGL UI elements */
static lv_obj_t *s_label_title = NULL;
static lv_obj_t *s_label_sensor1 = NULL;
static lv_obj_t *s_label_sensor2 = NULL;
static lv_obj_t *s_bar_sensor1 = NULL;
static lv_obj_t *s_bar_sensor2 = NULL;

/**
 * @brief Cycle the onboard WS2812 RGB LED through R, G, B colors.
 *
 * The ESP32 SuperMini boards have an addressable WS2812 LED on
 * BLINKY_WS2812_GPIO.  This task initialises the led_strip RMT
 * driver and cycles through red, green, and blue at 500 ms intervals.
 * Additional free GPIOs in BLINKY_GPIO_PINS are toggled as plain
 * push-pull outputs for any external LEDs.
 *
 * @param[in] arg  Unused FreeRTOS task argument (pass NULL).
 *
 * @sideeffects Configures GPIO output registers and the RMT peripheral,
 *              drives the WS2812 data line and external LED GPIOs.
 */
static void blinky_task(void *arg)
{
    /* ---- WS2812 addressable RGB LED on BLINKY_WS2812_GPIO ---- */
    led_strip_handle_t strip = NULL;
    led_strip_config_t strip_cfg = {
        .strip_gpio_num = BLINKY_WS2812_GPIO,
        .max_leds       = 1,
    };
    led_strip_rmt_config_t rmt_cfg = {
        .resolution_hz  = 10 * 1000 * 1000, /* 10 MHz */
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &strip));
    led_strip_clear(strip);

    /* RGB colour table: Red → Green → Blue */
    static const uint8_t k_colors[][3] = {
        { 25,  0,  0 },   /* Red   */
        {  0, 25,  0 },   /* Green */
        {  0,  0, 25 },   /* Blue  */
    };
    const int color_count = sizeof(k_colors) / sizeof(k_colors[0]);

#if BLINKY_GPIO_COUNT > 0
    /* ---- Plain GPIO LEDs for external wiring ---- */
    static const gpio_num_t k_gpios[] = BLINKY_GPIO_PINS;
    for (int i = 0; i < BLINKY_GPIO_COUNT; ++i) {
        gpio_config_t io_cfg = {
            .intr_type    = GPIO_INTR_DISABLE,
            .mode         = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << k_gpios[i]),
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
        };
        gpio_config(&io_cfg);
        gpio_set_level(k_gpios[i], 0);
    }
#endif

    ESP_LOGI(TAG, "blinky_task: WS2812 on GPIO%d, %d color(s), "
             "%d ext GPIO(s)", BLINKY_WS2812_GPIO, color_count,
             BLINKY_GPIO_COUNT);

    int c_idx = 0;
#if BLINKY_GPIO_COUNT > 0
    int g_idx = 0;
#endif
    while (1) {
        /* Set WS2812 colour */
        led_strip_set_pixel(strip, 0,
                            k_colors[c_idx][0],
                            k_colors[c_idx][1],
                            k_colors[c_idx][2]);
        led_strip_refresh(strip);
        ESP_LOGI(TAG, "blinky: WS2812 color %d (R=%d G=%d B=%d)",
                 c_idx, k_colors[c_idx][0], k_colors[c_idx][1],
                 k_colors[c_idx][2]);

#if BLINKY_GPIO_COUNT > 0
        /* Toggle current external GPIO LED ON */
        gpio_set_level(k_gpios[g_idx], 1);
        ESP_LOGI(TAG, "blinky: ext GPIO%d ON", (int)k_gpios[g_idx]);
#endif

        vTaskDelay(pdMS_TO_TICKS(500));

#if BLINKY_GPIO_COUNT > 0
        /* Turn off current external GPIO LED */
        gpio_set_level(k_gpios[g_idx], 0);
        g_idx = (g_idx + 1) % BLINKY_GPIO_COUNT;
#endif

        c_idx = (c_idx + 1) % color_count;
    }
}

/**
 * @brief Initialize ADC1 with two oneshot channels.
 *
 * @return ESP_OK on success, or an error code.
 *
 * @sideeffects Configures ADC1 hardware for oneshot sampling.
 */
static esp_err_t adc_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_SENSOR_UNIT,
    };
    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC unit init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ret = adc_oneshot_config_channel(s_adc_handle, ADC_SENSOR1_CHANNEL,
                                     &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC ch0 config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = adc_oneshot_config_channel(s_adc_handle, ADC_SENSOR2_CHANNEL,
                                     &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC ch1 config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "ADC initialized (CH%d=GPIO%d, CH%d=GPIO%d)",
             ADC_SENSOR1_CHANNEL, PIN_ADC_SENSOR1,
             ADC_SENSOR2_CHANNEL, PIN_ADC_SENSOR2);
    return ESP_OK;
}

/**
 * @brief Read a raw ADC value from the specified channel.
 *
 * @param[in]  channel  ADC channel to read.
 * @param[out] raw      Pointer to store the raw 12-bit value.
 * @return ESP_OK on success.
 */
static esp_err_t adc_read_channel(adc_channel_t channel, int *raw)
{
    return adc_oneshot_read(s_adc_handle, channel, raw);
}

/**
 * @brief Create the LVGL user interface.
 *
 * Builds a simple dashboard with a title, two sensor value labels,
 * and two progress bars showing the ADC readings.
 *
 * @sideeffects Creates LVGL objects on the active screen.
 */
static void ui_create(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a2e), LV_PART_MAIN);

    /* Title */
    s_label_title = lv_label_create(scr);
    lv_label_set_text(s_label_title, "Bobbycar-Steering");
    lv_obj_set_style_text_color(s_label_title, lv_color_hex(0xe94560),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(s_label_title, &lv_font_montserrat_24,
                               LV_PART_MAIN);
    lv_obj_align(s_label_title, LV_ALIGN_TOP_MID, 0, 15);

    /* Target info */
    lv_obj_t *label_target = lv_label_create(scr);
#if CONFIG_IDF_TARGET_ESP32C3
    lv_label_set_text(label_target, "ESP32-C3 | 160 MHz | Wi-Fi+BLE");
#elif CONFIG_IDF_TARGET_ESP32H2
    lv_label_set_text(label_target, "ESP32-H2 | 96 MHz | BLE+802.15.4");
#elif CONFIG_IDF_TARGET_ESP32C5
    lv_label_set_text(label_target, "ESP32-C5 | 240 MHz | Wi-Fi6+BLE");
#else
    lv_label_set_text(label_target, "Unknown target");
#endif
    lv_obj_set_style_text_color(label_target, lv_color_hex(0x888888),
                                LV_PART_MAIN);
    lv_obj_align(label_target, LV_ALIGN_TOP_MID, 0, 50);

    /* ---- Sensor 1 ---- */
    lv_obj_t *lbl1 = lv_label_create(scr);
    lv_label_set_text(lbl1, "Sensor 1");
    lv_obj_set_style_text_color(lbl1, lv_color_hex(0x0f3460), LV_PART_MAIN);
    lv_obj_align(lbl1, LV_ALIGN_LEFT_MID, 20, -50);

    s_label_sensor1 = lv_label_create(scr);
    lv_label_set_text(s_label_sensor1, "----");
    lv_obj_set_style_text_color(s_label_sensor1, lv_color_hex(0x16c79a),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(s_label_sensor1, &lv_font_montserrat_24,
                               LV_PART_MAIN);
    lv_obj_align(s_label_sensor1, LV_ALIGN_LEFT_MID, 120, -50);

    s_bar_sensor1 = lv_bar_create(scr);
    lv_obj_set_size(s_bar_sensor1, 300, 20);
    lv_bar_set_range(s_bar_sensor1, 0, 4095);
    lv_bar_set_value(s_bar_sensor1, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_sensor1, lv_color_hex(0x333333),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar_sensor1, lv_color_hex(0x16c79a),
                              LV_PART_INDICATOR);
    lv_obj_align(s_bar_sensor1, LV_ALIGN_LEFT_MID, 120, -25);

    /* ---- Sensor 2 ---- */
    lv_obj_t *lbl2 = lv_label_create(scr);
    lv_label_set_text(lbl2, "Sensor 2");
    lv_obj_set_style_text_color(lbl2, lv_color_hex(0x0f3460), LV_PART_MAIN);
    lv_obj_align(lbl2, LV_ALIGN_LEFT_MID, 20, 30);

    s_label_sensor2 = lv_label_create(scr);
    lv_label_set_text(s_label_sensor2, "----");
    lv_obj_set_style_text_color(s_label_sensor2, lv_color_hex(0xe94560),
                                LV_PART_MAIN);
    lv_obj_set_style_text_font(s_label_sensor2, &lv_font_montserrat_24,
                               LV_PART_MAIN);
    lv_obj_align(s_label_sensor2, LV_ALIGN_LEFT_MID, 120, 30);

    s_bar_sensor2 = lv_bar_create(scr);
    lv_obj_set_size(s_bar_sensor2, 300, 20);
    lv_bar_set_range(s_bar_sensor2, 0, 4095);
    lv_bar_set_value(s_bar_sensor2, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar_sensor2, lv_color_hex(0x333333),
                              LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar_sensor2, lv_color_hex(0xe94560),
                              LV_PART_INDICATOR);
    lv_obj_align(s_bar_sensor2, LV_ALIGN_LEFT_MID, 120, 55);
}

/**
 * @brief Update the UI with new ADC readings.
 *
 * @param[in] raw1  Raw 12-bit ADC value for sensor 1.
 * @param[in] raw2  Raw 12-bit ADC value for sensor 2.
 *
 * @sideeffects Updates LVGL label text and bar values.
 */
static void ui_update(int raw1, int raw2)
{
    char buf[16];

    snprintf(buf, sizeof(buf), "%d", raw1);
    lv_label_set_text(s_label_sensor1, buf);
    lv_bar_set_value(s_bar_sensor1, raw1, LV_ANIM_ON);

    snprintf(buf, sizeof(buf), "%d", raw2);
    lv_label_set_text(s_label_sensor2, buf);
    lv_bar_set_value(s_bar_sensor2, raw2, LV_ANIM_ON);
}

/**
 * @brief Sensor and display update task.
 *
 * Runs in its own FreeRTOS task to read ADC sensors and update
 * the LVGL display while BTstack occupies the main task.
 *
 * @param[in] arg  Unused FreeRTOS task argument (pass NULL).
 *
 * @sideeffects Reads ADC channels, updates LVGL widget values,
 *              runs LVGL timer handler.
 */
static void sensor_display_task(void *arg)
{
    int adc_raw1 = 0;
    int adc_raw2 = 0;
    TickType_t last_adc_read = 0;

    while (1) {
        /* Read ADC at ~20 Hz */
        TickType_t now = xTaskGetTickCount();
        if ((now - last_adc_read) >= pdMS_TO_TICKS(50)) {
            last_adc_read = now;

            if (adc_read_channel(ADC_SENSOR1_CHANNEL, &adc_raw1) == ESP_OK &&
                adc_read_channel(ADC_SENSOR2_CHANNEL, &adc_raw2) == ESP_OK) {
                ui_update(adc_raw1, adc_raw2);
            }
        }

        /* Run LVGL timer handler */
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/**
 * @brief Application entry point.
 *
 * Initializes peripherals (display, ADC, UI), starts FreeRTOS
 * tasks (blinky, sensor/display), then initializes Bluepad32
 * and enters the BTstack run loop (does not return).
 *
 * @sideeffects Initializes all system peripherals, creates LVGL UI,
 *              starts Bluetooth scanning, prints startup banner.
 */
void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG, "Bobbycar-Steering v0.4");

#if CONFIG_IDF_TARGET_ESP32C3
    ESP_LOGI(TAG, "Target: ESP32-C3 (RISC-V, 160 MHz, Wi-Fi + BLE 5)");
#elif CONFIG_IDF_TARGET_ESP32H2
    ESP_LOGI(TAG, "Target: ESP32-H2 (RISC-V, 96 MHz, BLE 5 + 802.15.4)");
#elif CONFIG_IDF_TARGET_ESP32C5
    ESP_LOGI(TAG, "Target: ESP32-C5 (RISC-V, 240 MHz, Wi-Fi 6 + BLE 5 + 802.15.4)");
#else
    ESP_LOGI(TAG, "Target: Unknown ESP32 variant");
#endif

    ESP_LOGI(TAG, "Cores: %d, Silicon revision: %d", chip_info.cores,
             chip_info.revision);
    ESP_LOGI(TAG, "Free heap: %lu bytes",
             (unsigned long)esp_get_free_heap_size());

    /* ---- Initialize display ---- */
    esp_err_t ret = display_init(
        PIN_DISPLAY_MOSI, PIN_DISPLAY_SCLK, PIN_DISPLAY_CS,
        PIN_DISPLAY_DC, PIN_DISPLAY_RST, PIN_DISPLAY_BL,
        DISPLAY_H_RES, DISPLAY_V_RES,
        DISPLAY_SPI_HOST, DISPLAY_SPI_FREQ_HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display init failed: %s", esp_err_to_name(ret));
        return;
    }

    /* ---- Initialize ADC ---- */
    ret = adc_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC init failed: %s", esp_err_to_name(ret));
        return;
    }

    /* ---- Create UI ---- */
    ui_create();

    /* ---- Initialize LittleFS storage ---- */
    ret = storage_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Storage init failed: %s (continuing without persistence)",
                 esp_err_to_name(ret));
    }

    /* ---- Start blinky task ---- */
    xTaskCreate(blinky_task, "blinky", 4096, NULL, 5, NULL);

    /* ---- Start sensor/display task ---- */
    xTaskCreate(sensor_display_task, "sensor_disp", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "System initialized. Starting Bluepad32...");

    /* ---- Initialize Bluepad32 + BTstack ---- */
    btstack_init();

    uni_platform_set_custom(get_my_platform());

    uni_init(0, NULL);

    /* BTstack run loop — does NOT return. */
    btstack_run_loop_execute();
}
