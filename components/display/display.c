/**
 * @file display.c
 * @brief ILI9488 4.0" SPI TFT display driver with XPT2046 touch
 *        and LVGL v9 integration.
 *
 * The ILI9488 operates in 18-bit RGB666 mode over SPI (3 bytes
 * per pixel).  LVGL renders in 16-bit RGB565 and this driver
 * converts to RGB666 in the flush callback before DMA transfer.
 *
 * Touch input uses the XPT2046 resistive touch controller on the
 * same SPI bus with a separate chip-select line.
 */

#include "display.h"

#include <string.h>
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "display";

/** Number of lines per partial render buffer. */
#ifndef LVGL_BUF_LINES
#define LVGL_BUF_LINES 6
#endif

/* ---- XPT2046 calibration (adjust after testing) ---- */
#define TOUCH_X_MIN     200
#define TOUCH_X_MAX     3700
#define TOUCH_Y_MIN     200
#define TOUCH_Y_MAX     3700
#define TOUCH_XY_SWAP   true
#define TOUCH_X_INVERT  false
#define TOUCH_Y_INVERT  true
#define TOUCH_Z_THRESH  400
#define TOUCH_SPI_FREQ  (1 * 1000 * 1000)

/* XPT2046 command bytes (12-bit, differential) */
#define XPT_CMD_X   0x90
#define XPT_CMD_Y   0xD0
#define XPT_CMD_Z1  0xB0

/* ---- Static state ---- */
static esp_lcd_panel_handle_t s_panel = NULL;
static esp_lcd_panel_io_handle_t s_panel_io = NULL;
static lv_display_t *s_lvgl_display = NULL;
static uint8_t *s_buf1 = NULL;
static uint8_t *s_buf2 = NULL;
static uint8_t *s_conv_buf = NULL;

static spi_device_handle_t s_touch_spi = NULL;
static lv_indev_t *s_touch_indev = NULL;
static int s_h_res = 0;
static int s_v_res = 0;
static bool s_last_pressed = false;
static int s_last_x = 0;
static int s_last_y = 0;

/* ================================================================== */
/*  XPT2046 Touch Controller                                          */
/* ================================================================== */

/**
 * @brief Read a 12-bit value from the XPT2046.
 *
 * @param[in] cmd  Command byte (channel select + mode bits).
 * @return 12-bit ADC value.
 *
 * @sideeffects Performs SPI transaction on touch device.
 */
static uint16_t xpt2046_read_reg(uint8_t cmd)
{
    uint8_t tx[3] = { cmd, 0x00, 0x00 };
    uint8_t rx[3] = { 0 };
    spi_transaction_t t = {
        .length = 24,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    spi_device_transmit(s_touch_spi, &t);
    return ((rx[1] << 8) | rx[2]) >> 3;
}

/**
 * @brief Read and map touch coordinates from XPT2046.
 *
 * @param[out] x  Mapped X coordinate (display pixels).
 * @param[out] y  Mapped Y coordinate (display pixels).
 * @return true if touch is active (pressure above threshold).
 *
 * @sideeffects Reads XPT2046 via SPI.
 */
static bool xpt2046_read_coords(int *x, int *y)
{
    uint16_t z1 = xpt2046_read_reg(XPT_CMD_Z1);
    if (z1 < TOUCH_Z_THRESH) {
        return false;
    }

    /* Average 4 samples for stability */
    uint32_t raw_x = 0, raw_y = 0;
    for (int i = 0; i < 4; i++) {
        raw_x += xpt2046_read_reg(XPT_CMD_X);
        raw_y += xpt2046_read_reg(XPT_CMD_Y);
    }
    raw_x /= 4;
    raw_y /= 4;

    int tx = (int)raw_x;
    int ty = (int)raw_y;

    if (TOUCH_XY_SWAP) { int tmp = tx; tx = ty; ty = tmp; }
    if (TOUCH_X_INVERT) { tx = TOUCH_X_MAX - (tx - TOUCH_X_MIN); }
    if (TOUCH_Y_INVERT) { ty = TOUCH_Y_MAX - (ty - TOUCH_Y_MIN); }

    if (tx < (int)TOUCH_X_MIN) tx = TOUCH_X_MIN;
    if (tx > (int)TOUCH_X_MAX) tx = TOUCH_X_MAX;
    if (ty < (int)TOUCH_Y_MIN) ty = TOUCH_Y_MIN;
    if (ty > (int)TOUCH_Y_MAX) ty = TOUCH_Y_MAX;

    *x = (tx - TOUCH_X_MIN) * s_h_res / (TOUCH_X_MAX - TOUCH_X_MIN);
    *y = (ty - TOUCH_Y_MIN) * s_v_res / (TOUCH_Y_MAX - TOUCH_Y_MIN);

    if (*x >= s_h_res) *x = s_h_res - 1;
    if (*y >= s_v_res) *y = s_v_res - 1;
    return true;
}

/**
 * @brief LVGL touch input device read callback.
 *
 * @sideeffects Reads XPT2046, updates LVGL input state.
 */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    int x, y;
    if (xpt2046_read_coords(&x, &y)) {
        data->point.x = (int32_t)x;
        data->point.y = (int32_t)y;
        data->state = LV_INDEV_STATE_PRESSED;
        s_last_pressed = true;
        s_last_x = x;
        s_last_y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        s_last_pressed = false;
    }
}

/* ================================================================== */
/*  Display Driver                                                     */
/* ================================================================== */

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
 * @brief LVGL flush callback — convert RGB565 to RGB666 and send.
 *
 * @sideeffects Converts pixel data, writes to ILI9488 via SPI DMA.
 */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map)
{
    int x_start = area->x1;
    int y_start = area->y1;
    int x_end = area->x2 + 1;
    int y_end = area->y2 + 1;
    int pixel_count = (x_end - x_start) * (y_end - y_start);

    /* Convert RGB565 → RGB666 (3 bytes per pixel) */
    const uint16_t *src = (const uint16_t *)px_map;
    uint8_t *dst = s_conv_buf;
    for (int i = 0; i < pixel_count; i++) {
        uint16_t c = src[i];
        *dst++ = (c >> 8) & 0xF8;
        *dst++ = (c >> 3) & 0xFC;
        *dst++ = (c << 3) & 0xF8;
    }

    esp_lcd_panel_draw_bitmap(s_panel, x_start, y_start,
                              x_end, y_end, s_conv_buf);
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

esp_err_t display_init(int mosi_pin, int miso_pin, int sclk_pin,
                       int cs_pin, int dc_pin, int rst_pin, int bl_pin,
                       int h_res, int v_res,
                       spi_host_device_t spi_host, int spi_freq)
{
    esp_err_t ret;
    s_h_res = h_res;
    s_v_res = v_res;

    ESP_LOGI(TAG, "Initializing ILI9488 display (%dx%d)", h_res, v_res);

    /* ---- 1. Configure SPI bus (MISO needed for XPT2046 touch) ---- */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = mosi_pin,
        .miso_io_num = miso_pin,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = h_res * LVGL_BUF_LINES * 3,
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

    /* ---- 3. Create LCD panel (ST7789 driver, ILI9488-compatible) ---- */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = rst_pin,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 18,
    };
    ret = esp_lcd_new_panel_st7789(s_panel_io, &panel_cfg, &s_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD panel init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ---- 4. Initialize panel ---- */
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);

    /* ILI9488: inversion OFF (ST7789 driver enables it by default) */
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

    /* Allocate LVGL draw buffers (RGB565 — converted in flush cb) */
    size_t buf_size = h_res * LVGL_BUF_LINES * sizeof(uint16_t);
    s_buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    /* Allocate RGB666 conversion buffer (3 bytes per pixel) */
    size_t conv_size = h_res * LVGL_BUF_LINES * 3;
    s_conv_buf = heap_caps_malloc(conv_size,
                                  MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!s_buf1 || !s_buf2 || !s_conv_buf) {
        ESP_LOGE(TAG, "Failed to allocate display buffers"
                 " (%u + %u bytes)",
                 (unsigned)(buf_size * 2), (unsigned)conv_size);
        return ESP_ERR_NO_MEM;
    }

    /* Create LVGL display (RGB565 — conversion happens in flush) */
    s_lvgl_display = lv_display_create(h_res, v_res);
    lv_display_set_flush_cb(s_lvgl_display, lvgl_flush_cb);
    lv_display_set_buffers(s_lvgl_display, s_buf1, s_buf2, buf_size,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(s_lvgl_display, LV_COLOR_FORMAT_RGB565);

    ESP_LOGI(TAG, "Display initialized (%d-line bufs, 18-bit RGB666)",
             LVGL_BUF_LINES);
    return ESP_OK;
}

/**
 * @brief Initialize the XPT2046 resistive touch controller.
 *
 * Adds an SPI device to the existing bus (initialized by
 * display_init) and registers an LVGL pointer input device.
 *
 * @param[in] spi_host  SPI host (same as display).
 * @param[in] cs_pin    GPIO number for touch CS.
 * @param[in] irq_pin   GPIO for touch IRQ, or -1 for polling.
 * @param[in] h_res     Horizontal resolution (for mapping).
 * @param[in] v_res     Vertical resolution (for mapping).
 * @return ESP_OK on success, or an error code.
 *
 * @sideeffects Adds SPI device, creates LVGL input device.
 */
esp_err_t touch_init(spi_host_device_t spi_host, int cs_pin,
                     int irq_pin, int h_res, int v_res)
{
    s_h_res = h_res;
    s_v_res = v_res;

    ESP_LOGI(TAG, "Initializing XPT2046 touch (CS=%d, IRQ=%d)",
             cs_pin, irq_pin);

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = TOUCH_SPI_FREQ,
        .mode = 0,
        .spics_io_num = cs_pin,
        .queue_size = 1,
    };
    esp_err_t ret = spi_bus_add_device(spi_host, &dev_cfg, &s_touch_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Touch SPI device init failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    if (irq_pin >= 0) {
        gpio_config_t irq_cfg = {
            .pin_bit_mask = 1ULL << irq_pin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&irq_cfg);
    }

    s_touch_indev = lv_indev_create();
    lv_indev_set_type(s_touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_touch_indev, touch_read_cb);

    ESP_LOGI(TAG, "Touch initialized (XPT2046, %dx%d)", h_res, v_res);
    return ESP_OK;
}

lv_display_t *display_get_lvgl_display(void)
{
    return s_lvgl_display;
}

/**
 * @brief Get the LVGL touch input device handle.
 *
 * @return Pointer to the LVGL indev, or NULL if not initialized.
 */
lv_indev_t *touch_get_indev(void)
{
    return s_touch_indev;
}

/**
 * @brief Get the last touch point read by the driver.
 *
 * @param[out] x  Last X coordinate.
 * @param[out] y  Last Y coordinate.
 * @return true if touch is currently pressed.
 */
bool touch_get_last_point(int *x, int *y)
{
    *x = s_last_x;
    *y = s_last_y;
    return s_last_pressed;
}
