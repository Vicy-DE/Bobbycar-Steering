/**
 * @file display.h
 * @brief Display and touch driver API.
 *
 * Two compile-time variants (selected by DISPLAY_VARIANT):
 *   0 — ILI9488 4.0" SPI TFT + XPT2046 resistive SPI touch
 *   1 — ST7796S 4.0" SPI TFT + FT6236 capacitive I2C touch
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "lvgl.h"

#ifndef DISPLAY_VARIANT
#define DISPLAY_VARIANT 0
#endif

#if DISPLAY_VARIANT == 0
/* ================================================================== */
/*  Variant 0 — ILI9488 (18-bit RGB666) + XPT2046 (SPI)              */
/* ================================================================== */

/**
 * @brief Initialize the ILI9488 display and LVGL integration.
 *
 * Configures SPI bus with MISO (needed for XPT2046 touch),
 * initializes the ILI9488 in 18-bit RGB666 mode, sets up LVGL
 * with double-buffered partial rendering and an RGB565-to-RGB666
 * conversion buffer for DMA transfer.
 *
 * @param[in] mosi_pin   GPIO for SPI MOSI.
 * @param[in] miso_pin   GPIO for SPI MISO (touch read-back).
 * @param[in] sclk_pin   GPIO for SPI SCLK.
 * @param[in] cs_pin     GPIO for display SPI CS.
 * @param[in] dc_pin     GPIO for Data/Command select.
 * @param[in] rst_pin    GPIO for hardware reset.
 * @param[in] bl_pin     GPIO for backlight control.
 * @param[in] h_res      Horizontal resolution (landscape).
 * @param[in] v_res      Vertical resolution (landscape).
 * @param[in] spi_host   SPI host (SPI2_HOST).
 * @param[in] spi_freq   SPI clock frequency in Hz.
 * @return ESP_OK on success.
 *
 * @sideeffects Configures SPI2 bus, GPIO pins, allocates DMA
 *              buffers, initializes LVGL library.
 */
esp_err_t display_init(int mosi_pin, int miso_pin, int sclk_pin,
                       int cs_pin, int dc_pin, int rst_pin, int bl_pin,
                       int h_res, int v_res,
                       spi_host_device_t spi_host, int spi_freq);

/**
 * @brief Initialize the XPT2046 resistive touch controller.
 *
 * Adds an SPI device to the existing display bus and registers
 * an LVGL pointer input device.
 *
 * @param[in] spi_host  SPI host (same bus as display).
 * @param[in] cs_pin    GPIO for touch CS.
 * @param[in] irq_pin   GPIO for touch IRQ, or -1 for polling.
 * @param[in] h_res     Horizontal resolution (for mapping).
 * @param[in] v_res     Vertical resolution (for mapping).
 * @return ESP_OK on success.
 *
 * @sideeffects Adds SPI device, creates LVGL input device.
 */
esp_err_t touch_init(spi_host_device_t spi_host, int cs_pin,
                     int irq_pin, int h_res, int v_res);

#elif DISPLAY_VARIANT == 1
/* ================================================================== */
/*  Variant 1 — ST7796S (16-bit RGB565) + FT6236 (I2C)               */
/* ================================================================== */

/**
 * @brief Initialize the ST7796S display and LVGL integration.
 *
 * Configures SPI bus (write-only, no MISO), initializes the
 * ST7796S in native 16-bit RGB565 mode, sets up LVGL with
 * double-buffered partial rendering with zero-copy DMA transfer
 * (no colour conversion needed).
 *
 * @param[in] mosi_pin   GPIO for SPI MOSI.
 * @param[in] sclk_pin   GPIO for SPI SCLK.
 * @param[in] cs_pin     GPIO for display SPI CS.
 * @param[in] dc_pin     GPIO for Data/Command select.
 * @param[in] rst_pin    GPIO for hardware reset.
 * @param[in] bl_pin     GPIO for backlight control.
 * @param[in] h_res      Horizontal resolution (landscape).
 * @param[in] v_res      Vertical resolution (landscape).
 * @param[in] spi_host   SPI host (SPI2_HOST).
 * @param[in] spi_freq   SPI clock frequency in Hz.
 * @return ESP_OK on success.
 *
 * @sideeffects Configures SPI2 bus, GPIO pins, allocates DMA
 *              buffers, initializes LVGL library.
 */
esp_err_t display_init(int mosi_pin, int sclk_pin,
                       int cs_pin, int dc_pin, int rst_pin, int bl_pin,
                       int h_res, int v_res,
                       spi_host_device_t spi_host, int spi_freq);

/**
 * @brief Initialize the FT6236 capacitive touch controller.
 *
 * Creates a new I2C master bus and adds the FT6236 device at
 * address 0x38. Registers an LVGL pointer input device.
 *
 * @param[in] sda_pin   GPIO for I2C SDA.
 * @param[in] scl_pin   GPIO for I2C SCL.
 * @param[in] int_pin   GPIO for touch INT, or -1 unused.
 * @param[in] h_res     Horizontal resolution (for mapping).
 * @param[in] v_res     Vertical resolution (for mapping).
 * @return ESP_OK on success.
 *
 * @sideeffects Configures I2C bus, creates LVGL input device.
 */
esp_err_t touch_init(int sda_pin, int scl_pin, int int_pin,
                     int h_res, int v_res);

#endif /* DISPLAY_VARIANT */

/**
 * @brief Get the LVGL display handle.
 *
 * @return Pointer to the LVGL display, or NULL if not initialized.
 */
lv_display_t *display_get_lvgl_display(void);

/**
 * @brief Get the LVGL touch input device handle.
 *
 * @return Pointer to the LVGL indev, or NULL if not initialized.
 */
lv_indev_t *touch_get_indev(void);

/**
 * @brief Get the last touch point read by the driver.
 *
 * @param[out] x  Last X coordinate.
 * @param[out] y  Last Y coordinate.
 * @return true if touch is currently pressed.
 */
bool touch_get_last_point(int *x, int *y);
