/**
 * @file display.c
 * @brief ST7796S 3.5" SPI TFT display driver with LVGL v9 integration.
 *
 * Uses ESP-IDF esp_lcd panel IO for SPI communication and integrates
 * with LVGL v9 display driver API for partial-rendering updates.
 */

#include "display.h"

#include <string.h>
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "display";

/** Number of lines per partial render buffer. */
#ifndef LVGL_BUF_LINES
#define LVGL_BUF_LINES 20
#endif

static esp_lcd_panel_handle_t s_panel = NULL;
static esp_lcd_panel_io_handle_t s_panel_io = NULL;
static lv_display_t *s_lvgl_display = NULL;
static uint8_t *s_buf1 = NULL;
static uint8_t *s_buf2 = NULL;

/**
 * @brief Callback when SPI color transfer completes.
 *
 * Notifies LVGL that the flush is finished so the next area can
 * be rendered.
 *
 * @sideeffects Calls lv_display_flush_ready().
 */
static bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                               esp_lcd_panel_io_event_data_t *edata,
                               void *user_ctx)
{
    if (s_lvgl_display) {
        lv_display_flush_ready(s_lvgl_display);
    }
    return false;
}

/**
 * @brief LVGL flush callback — sends pixel data to the display.
 *
 * @sideeffects Writes pixel data to the ST7796S via SPI DMA.
 */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map)
{
    int x_start = area->x1;
    int y_start = area->y1;
    int x_end = area->x2 + 1;
    int y_end = area->y2 + 1;

    esp_lcd_panel_draw_bitmap(s_panel, x_start, y_start,
                              x_end, y_end, px_map);
}

/**
 * @brief LVGL tick callback — returns milliseconds since boot.
 *
 * @return Current tick count in milliseconds.
 */
static uint32_t lvgl_tick_cb(void)
{
    return (uint32_t)(esp_log_timestamp());
}

/**
 * @brief Configure the backlight GPIO and turn it on.
 *
 * @param[in] bl_pin  GPIO number for backlight control.
 *
 * @sideeffects Configures GPIO as output and sets it high.
 */
static void backlight_init(int bl_pin)
{
    if (bl_pin < 0) {
        return;
    }
    gpio_config_t bl_cfg = {
        .pin_bit_mask = 1ULL << bl_pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl_cfg);
    gpio_set_level(bl_pin, 1);
}

esp_err_t display_init(int mosi_pin, int sclk_pin, int cs_pin,
                       int dc_pin, int rst_pin, int bl_pin,
                       int h_res, int v_res,
                       spi_host_device_t spi_host, int spi_freq)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing ST7796S display (%dx%d)", h_res, v_res);

    /* ---- 1. Configure SPI bus ---- */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = mosi_pin,
        .miso_io_num = -1,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = h_res * 80 * sizeof(uint16_t),
    };
    ret = spi_bus_initialize(spi_host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ---- 2. Create LCD panel IO ---- */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = dc_pin,
        .cs_gpio_num = cs_pin,
        .pclk_hz = spi_freq,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_flush_ready,
        .user_ctx = NULL,
    };
    ret = esp_lcd_new_panel_io_spi(spi_host, &io_cfg, &s_panel_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD panel IO init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ---- 3. Create LCD panel (ST7789 driver works for ST7796S) ---- */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = rst_pin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
    };
    ret = esp_lcd_new_panel_st7789(s_panel_io, &panel_cfg, &s_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD panel init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ---- 4. Initialize panel ---- */
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);

    /* ST7796S-specific: inversion OFF (ST7789 driver enables it) */
    esp_lcd_panel_invert_color(s_panel, false);

    /* Landscape rotation: swap X/Y */
    esp_lcd_panel_swap_xy(s_panel, true);
    esp_lcd_panel_mirror(s_panel, false, true);

    /* Turn on display */
    esp_lcd_panel_disp_on_off(s_panel, true);

    /* ---- 5. Backlight ---- */
    backlight_init(bl_pin);

    /* ---- 6. Initialize LVGL ---- */
    lv_init();
    lv_tick_set_cb(lvgl_tick_cb);

    /* Allocate draw buffers */
    size_t buf_size = h_res * LVGL_BUF_LINES * sizeof(uint16_t);
    s_buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_buf1 || !s_buf2) {
        ESP_LOGE(TAG, "Failed to allocate LVGL draw buffers (%u bytes each)",
                 (unsigned)buf_size);
        return ESP_ERR_NO_MEM;
    }

    /* Create LVGL display */
    s_lvgl_display = lv_display_create(h_res, v_res);
    lv_display_set_flush_cb(s_lvgl_display, lvgl_flush_cb);
    lv_display_set_buffers(s_lvgl_display, s_buf1, s_buf2, buf_size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(s_lvgl_display, LV_COLOR_FORMAT_RGB565);

    ESP_LOGI(TAG, "Display initialized successfully");
    return ESP_OK;
}

lv_display_t *display_get_lvgl_display(void)
{
    return s_lvgl_display;
}
