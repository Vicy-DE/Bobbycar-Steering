/**
 * @file display.c
 * @brief Display and touch driver with LVGL v9 integration.
 *
 * Two compile-time variants (DISPLAY_VARIANT):
 *
 *   0 — ILI9488 4.0" SPI TFT (18-bit RGB666) + XPT2046 resistive
 *       SPI touch on the same bus (needs MISO).
 *       RAM: 2×5760 (LVGL) + 8640 (conv) = 20160 bytes.
 *       Flush: RGB565 → RGB666 conversion then DMA.
 *
 *   1 — ST7796S 4.0" SPI TFT (16-bit RGB565) + FT6236 capacitive
 *       I2C touch on a separate bus (no SPI MISO needed).
 *       RAM: 2×9600 (LVGL) = 19200 bytes.  (960 bytes LESS)
 *       Flush: zero-copy DMA directly from LVGL buffer.
 *       Throughput: 33% faster (2 vs 3 bytes/pixel over SPI).
 *       Touch: I2C bus — no contention with display SPI.
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

#if DISPLAY_VARIANT == 1
#include "driver/i2c_master.h"
#endif

static const char *TAG = "display";

#ifndef DISPLAY_VARIANT
#define DISPLAY_VARIANT 0
#endif

/* ---- Buffer sizing per variant ---- */
#if DISPLAY_VARIANT == 0
#define LVGL_BUF_LINES  6   /* ILI9488: 2×5760 + 8640 = 20160 bytes */
#elif DISPLAY_VARIANT == 1
#define LVGL_BUF_LINES  10  /* ST7796S: 2×9600 = 19200 bytes        */
#endif

/* ---- Shared static state ---- */
static esp_lcd_panel_handle_t    s_panel        = NULL;
static esp_lcd_panel_io_handle_t s_panel_io     = NULL;
static lv_display_t             *s_lvgl_display = NULL;
static uint8_t                  *s_buf1         = NULL;
static uint8_t                  *s_buf2         = NULL;
static lv_indev_t               *s_touch_indev  = NULL;
static int  s_h_res        = 0;
static int  s_v_res        = 0;
static bool s_last_pressed = false;
static int  s_last_x       = 0;
static int  s_last_y       = 0;

/* ================================================================== */
#if DISPLAY_VARIANT == 0
/* ================================================================== */
/*  Variant 0 — ILI9488 + XPT2046                                     */
/* ================================================================== */

static uint8_t *s_conv_buf = NULL;
static spi_device_handle_t s_touch_spi = NULL;

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
        .length    = 24,
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

    if (TOUCH_XY_SWAP)   { int tmp = tx; tx = ty; ty = tmp; }
    if (TOUCH_X_INVERT)  { tx = TOUCH_X_MAX - (tx - TOUCH_X_MIN); }
    if (TOUCH_Y_INVERT)  { ty = TOUCH_Y_MAX - (ty - TOUCH_Y_MIN); }

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
 * @brief LVGL touch input device read callback (XPT2046).
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
        data->state   = LV_INDEV_STATE_PRESSED;
        s_last_pressed = true;
        s_last_x = x;
        s_last_y = y;
    } else {
        data->state    = LV_INDEV_STATE_RELEASED;
        s_last_pressed = false;
    }
}

/* ================================================================== */
#elif DISPLAY_VARIANT == 1
/* ================================================================== */
/*  Variant 1 — ST7796S + FT6236                                      */
/* ================================================================== */

static i2c_master_bus_handle_t  s_touch_i2c_bus = NULL;
static i2c_master_dev_handle_t  s_touch_i2c_dev = NULL;

#define FT6236_ADDR       0x38
#define FT6236_REG_TD     0x02   /* Touch data status register */

/* FT6236 coordinate mapping — adjust for panel orientation */
#define FT_SWAP_XY    true
#define FT_INVERT_X   false
#define FT_INVERT_Y   false
#define FT_RAW_X_MAX  319
#define FT_RAW_Y_MAX  479

/**
 * @brief Read touch coordinates from the FT6236 via I2C.
 *
 * @param[out] x  Mapped X coordinate (display pixels).
 * @param[out] y  Mapped Y coordinate (display pixels).
 * @return true if a finger is currently touching the screen.
 *
 * @sideeffects Reads FT6236 registers via I2C.
 */
static bool ft6236_read_touch(int *x, int *y)
{
    uint8_t reg = FT6236_REG_TD;
    uint8_t data[5];
    esp_err_t ret = i2c_master_transmit_receive(
        s_touch_i2c_dev, &reg, 1, data, sizeof(data), 50);
    if (ret != ESP_OK) {
        return false;
    }

    uint8_t num_touches = data[0] & 0x0F;
    if (num_touches == 0 || num_touches > 2) {
        return false;
    }

    /* First touch point */
    int raw_x = ((data[1] & 0x0F) << 8) | data[2];
    int raw_y = ((data[3] & 0x0F) << 8) | data[4];

    int tx = raw_x;
    int ty = raw_y;

    if (FT_SWAP_XY)   { int tmp = tx; tx = ty; ty = tmp; }
    if (FT_INVERT_X)  { tx = s_h_res - 1 - tx; }
    if (FT_INVERT_Y)  { ty = s_v_res - 1 - ty; }

    if (tx < 0)        tx = 0;
    if (tx >= s_h_res) tx = s_h_res - 1;
    if (ty < 0)        ty = 0;
    if (ty >= s_v_res) ty = s_v_res - 1;

    *x = tx;
    *y = ty;
    return true;
}

/**
 * @brief LVGL touch input device read callback (FT6236).
 *
 * @sideeffects Reads FT6236, updates LVGL input state.
 */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    int x, y;
    if (ft6236_read_touch(&x, &y)) {
        data->point.x = (int32_t)x;
        data->point.y = (int32_t)y;
        data->state   = LV_INDEV_STATE_PRESSED;
        s_last_pressed = true;
        s_last_x = x;
        s_last_y = y;
    } else {
        data->state    = LV_INDEV_STATE_RELEASED;
        s_last_pressed = false;
    }
}

#endif /* DISPLAY_VARIANT */

/* ================================================================== */
/*  Shared Display Driver Code                                         */
/* ================================================================== */

/**
 * @brief Callback when SPI colour transfer completes.
 *
 * Notifies LVGL that the flush is finished so the next area can
 * be rendered into the other buffer (double-buffered DMA).
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

#if DISPLAY_VARIANT == 0
/**
 * @brief LVGL flush callback — convert RGB565 to RGB666 and DMA.
 *
 * ILI9488 requires 18-bit colour.  LVGL renders 16-bit RGB565;
 * this callback expands each pixel to 3 bytes (RGB666) into a
 * DMA-capable conversion buffer then sends it.
 *
 * @sideeffects Converts pixel data, writes to ILI9488 via SPI DMA.
 */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map)
{
    int x_start     = area->x1;
    int y_start     = area->y1;
    int x_end       = area->x2 + 1;
    int y_end       = area->y2 + 1;
    int pixel_count = (x_end - x_start) * (y_end - y_start);

    /* Convert RGB565 → RGB666 (3 bytes per pixel) */
    const uint16_t *src = (const uint16_t *)px_map;
    uint8_t *dst = s_conv_buf;
    for (int i = 0; i < pixel_count; i++) {
        uint16_t c = src[i];
        *dst++ = (c >> 8) & 0xF8;   /* R */
        *dst++ = (c >> 3) & 0xFC;   /* G */
        *dst++ = (c << 3) & 0xF8;   /* B */
    }

    esp_lcd_panel_draw_bitmap(s_panel, x_start, y_start,
                              x_end, y_end, s_conv_buf);
}
#elif DISPLAY_VARIANT == 1
/**
 * @brief LVGL flush callback — zero-copy DMA for RGB565.
 *
 * ST7796S accepts native 16-bit RGB565 over SPI.  The LVGL
 * buffer is already in the correct format and DMA-capable,
 * so we pass it directly — no conversion, no extra buffer.
 *
 * @sideeffects Writes to ST7796S via SPI DMA.
 */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map)
{
    esp_lcd_panel_draw_bitmap(s_panel,
                              area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1,
                              px_map);
}
#endif

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
        .pin_bit_mask  = 1ULL << bl_pin,
        .mode          = GPIO_MODE_OUTPUT,
        .pull_up_en    = GPIO_PULLUP_DISABLE,
        .pull_down_en  = GPIO_PULLDOWN_DISABLE,
        .intr_type     = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl_cfg);
    gpio_set_level(bl_pin, 1);
}

/* ================================================================== */
/*  display_init — variant-specific                                    */
/* ================================================================== */

#if DISPLAY_VARIANT == 0
esp_err_t display_init(int mosi_pin, int miso_pin, int sclk_pin,
                       int cs_pin, int dc_pin, int rst_pin, int bl_pin,
                       int h_res, int v_res,
                       spi_host_device_t spi_host, int spi_freq)
{
    esp_err_t ret;
    s_h_res = h_res;
    s_v_res = v_res;

    ESP_LOGI(TAG, "Initializing ILI9488 display (%dx%d)", h_res, v_res);

    /* ---- 1. SPI bus (MISO needed for XPT2046) ---- */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = mosi_pin,
        .miso_io_num     = miso_pin,
        .sclk_io_num     = sclk_pin,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = h_res * LVGL_BUF_LINES * 3,
    };
    ret = spi_bus_initialize(spi_host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ---- 2. LCD panel IO ---- */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num         = dc_pin,
        .cs_gpio_num         = cs_pin,
        .pclk_hz             = spi_freq,
        .lcd_cmd_bits        = 8,
        .lcd_param_bits      = 8,
        .spi_mode            = 0,
        .trans_queue_depth    = 10,
        .on_color_trans_done = notify_flush_ready,
        .user_ctx            = NULL,
    };
    ret = esp_lcd_new_panel_io_spi(spi_host, &io_cfg, &s_panel_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD panel IO init failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    /* ---- 3. LCD panel (ST7789 driver, ILI9488-compatible) ---- */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num  = rst_pin,
        .rgb_ele_order   = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian     = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel  = 18,
    };
    ret = esp_lcd_new_panel_st7789(s_panel_io, &panel_cfg, &s_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD panel init failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    /* ---- 4. Panel init ---- */
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    esp_lcd_panel_invert_color(s_panel, false);
    esp_lcd_panel_swap_xy(s_panel, true);
    esp_lcd_panel_mirror(s_panel, false, true);
    esp_lcd_panel_disp_on_off(s_panel, true);

    /* ---- 5. Backlight ---- */
    backlight_init(bl_pin);

    /* ---- 6. LVGL ---- */
    lv_init();
    lv_tick_set_cb(lvgl_tick_cb);

    size_t buf_size  = h_res * LVGL_BUF_LINES * sizeof(uint16_t);
    size_t conv_size = h_res * LVGL_BUF_LINES * 3;

    s_buf1     = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_buf2     = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_conv_buf = heap_caps_malloc(conv_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!s_buf1 || !s_buf2 || !s_conv_buf) {
        ESP_LOGE(TAG, "Failed to allocate display buffers"
                 " (2x%u + %u bytes)",
                 (unsigned)buf_size, (unsigned)conv_size);
        return ESP_ERR_NO_MEM;
    }

    s_lvgl_display = lv_display_create(h_res, v_res);
    lv_display_set_flush_cb(s_lvgl_display, lvgl_flush_cb);
    lv_display_set_buffers(s_lvgl_display, s_buf1, s_buf2,
                           buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(s_lvgl_display, LV_COLOR_FORMAT_RGB565);

    ESP_LOGI(TAG, "Display ready (ILI9488, %d-line bufs, 18-bit RGB666, "
             "total %u bytes)",
             LVGL_BUF_LINES, (unsigned)(buf_size * 2 + conv_size));
    return ESP_OK;
}

#elif DISPLAY_VARIANT == 1
esp_err_t display_init(int mosi_pin, int sclk_pin,
                       int cs_pin, int dc_pin, int rst_pin, int bl_pin,
                       int h_res, int v_res,
                       spi_host_device_t spi_host, int spi_freq)
{
    esp_err_t ret;
    s_h_res = h_res;
    s_v_res = v_res;

    ESP_LOGI(TAG, "Initializing ST7796S display (%dx%d)", h_res, v_res);

    /* ---- 1. SPI bus (write-only, no MISO needed) ---- */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = mosi_pin,
        .miso_io_num     = -1,
        .sclk_io_num     = sclk_pin,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = h_res * LVGL_BUF_LINES * sizeof(uint16_t),
    };
    ret = spi_bus_initialize(spi_host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ---- 2. LCD panel IO ---- */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num         = dc_pin,
        .cs_gpio_num         = cs_pin,
        .pclk_hz             = spi_freq,
        .lcd_cmd_bits        = 8,
        .lcd_param_bits      = 8,
        .spi_mode            = 0,
        .trans_queue_depth    = 10,
        .on_color_trans_done = notify_flush_ready,
        .user_ctx            = NULL,
    };
    ret = esp_lcd_new_panel_io_spi(spi_host, &io_cfg, &s_panel_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD panel IO init failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    /* ---- 3. LCD panel (ST7789 driver, ST7796S-compatible) ---- */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num  = rst_pin,
        .rgb_ele_order   = LCD_RGB_ELEMENT_ORDER_BGR,
        .data_endian     = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel  = 16,
    };
    ret = esp_lcd_new_panel_st7789(s_panel_io, &panel_cfg, &s_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD panel init failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    /* ---- 4. Panel init ---- */
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    /* ST7796S needs inversion ON for correct colours */
    esp_lcd_panel_invert_color(s_panel, true);
    esp_lcd_panel_swap_xy(s_panel, true);
    esp_lcd_panel_mirror(s_panel, false, true);
    esp_lcd_panel_disp_on_off(s_panel, true);

    /* ---- 5. Backlight ---- */
    backlight_init(bl_pin);

    /* ---- 6. LVGL (zero-copy: DMA straight from LVGL buffers) ---- */
    lv_init();
    lv_tick_set_cb(lvgl_tick_cb);

    size_t buf_size = h_res * LVGL_BUF_LINES * sizeof(uint16_t);

    s_buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!s_buf1 || !s_buf2) {
        ESP_LOGE(TAG, "Failed to allocate display buffers (2x%u bytes)",
                 (unsigned)buf_size);
        return ESP_ERR_NO_MEM;
    }

    s_lvgl_display = lv_display_create(h_res, v_res);
    lv_display_set_flush_cb(s_lvgl_display, lvgl_flush_cb);
    lv_display_set_buffers(s_lvgl_display, s_buf1, s_buf2,
                           buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(s_lvgl_display, LV_COLOR_FORMAT_RGB565);

    ESP_LOGI(TAG, "Display ready (ST7796S, %d-line bufs, 16-bit RGB565, "
             "zero-copy DMA, total %u bytes)",
             LVGL_BUF_LINES, (unsigned)(buf_size * 2));
    return ESP_OK;
}
#endif /* DISPLAY_VARIANT */

/* ================================================================== */
/*  touch_init — variant-specific                                      */
/* ================================================================== */

#if DISPLAY_VARIANT == 0
esp_err_t touch_init(spi_host_device_t spi_host, int cs_pin,
                     int irq_pin, int h_res, int v_res)
{
    s_h_res = h_res;
    s_v_res = v_res;

    ESP_LOGI(TAG, "Initializing XPT2046 touch (CS=%d, IRQ=%d)",
             cs_pin, irq_pin);

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = TOUCH_SPI_FREQ,
        .mode           = 0,
        .spics_io_num   = cs_pin,
        .queue_size     = 1,
    };
    esp_err_t ret = spi_bus_add_device(spi_host, &dev_cfg, &s_touch_spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Touch SPI device init failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    if (irq_pin >= 0) {
        gpio_config_t irq_cfg = {
            .pin_bit_mask  = 1ULL << irq_pin,
            .mode          = GPIO_MODE_INPUT,
            .pull_up_en    = GPIO_PULLUP_ENABLE,
            .pull_down_en  = GPIO_PULLDOWN_DISABLE,
            .intr_type     = GPIO_INTR_DISABLE,
        };
        gpio_config(&irq_cfg);
    }

    s_touch_indev = lv_indev_create();
    lv_indev_set_type(s_touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_touch_indev, touch_read_cb);

    ESP_LOGI(TAG, "Touch ready (XPT2046, %dx%d)", h_res, v_res);
    return ESP_OK;
}

#elif DISPLAY_VARIANT == 1
esp_err_t touch_init(int sda_pin, int scl_pin, int int_pin,
                     int h_res, int v_res)
{
    s_h_res = h_res;
    s_v_res = v_res;

    ESP_LOGI(TAG, "Initializing FT6236 touch (SDA=%d, SCL=%d, INT=%d)",
             sda_pin, scl_pin, int_pin);

    /* ---- I2C master bus ---- */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port                = I2C_NUM_0,
        .sda_io_num              = sda_pin,
        .scl_io_num              = scl_pin,
        .clk_source              = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt       = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &s_touch_i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ---- FT6236 device (addr 0x38) ---- */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = FT6236_ADDR,
        .scl_speed_hz    = 400000,
    };
    ret = i2c_master_bus_add_device(s_touch_i2c_bus, &dev_cfg,
                                    &s_touch_i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FT6236 device add failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    /* ---- INT pin (optional) ---- */
    if (int_pin >= 0) {
        gpio_config_t int_cfg = {
            .pin_bit_mask  = 1ULL << int_pin,
            .mode          = GPIO_MODE_INPUT,
            .pull_up_en    = GPIO_PULLUP_ENABLE,
            .pull_down_en  = GPIO_PULLDOWN_DISABLE,
            .intr_type     = GPIO_INTR_DISABLE,
        };
        gpio_config(&int_cfg);
    }

    s_touch_indev = lv_indev_create();
    lv_indev_set_type(s_touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_touch_indev, touch_read_cb);

    ESP_LOGI(TAG, "Touch ready (FT6236, I2C 0x%02X, %dx%d)",
             FT6236_ADDR, h_res, v_res);
    return ESP_OK;
}
#endif /* DISPLAY_VARIANT */

/* ================================================================== */
/*  Shared getter functions                                            */
/* ================================================================== */

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
