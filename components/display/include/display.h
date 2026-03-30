/**
 * @file display.h
 * @brief Display and touch driver API for ILI9488 4.0" SPI TFT
 *        with XPT2046 resistive touch.
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "lvgl.h"

/**
 * @brief Initialize the ILI9488 display and LVGL integration.
 *
 * Configures SPI bus, initializes the ILI9488 display controller
 * in 18-bit RGB666 mode, sets up LVGL display driver with
 * partial rendering buffers and RGB565-to-RGB666 conversion,
 * and turns on the backlight.
 *
 * @param[in] mosi_pin   GPIO number for SPI MOSI.
 * @param[in] miso_pin   GPIO number for SPI MISO (for touch).
 * @param[in] sclk_pin   GPIO number for SPI SCLK.
 * @param[in] cs_pin     GPIO number for display SPI CS.
 * @param[in] dc_pin     GPIO number for Data/Command select.
 * @param[in] rst_pin    GPIO number for hardware reset.
 * @param[in] bl_pin     GPIO number for backlight control.
 * @param[in] h_res      Horizontal resolution (after rotation).
 * @param[in] v_res      Vertical resolution (after rotation).
 * @param[in] spi_host   SPI host (SPI2_HOST).
 * @param[in] spi_freq   SPI clock frequency in Hz.
 * @return ESP_OK on success, or an error code.
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
 * Adds an SPI device to the existing bus (initialized by
 * display_init) and registers an LVGL pointer input device.
 *
 * @param[in] spi_host  SPI host (same bus as display).
 * @param[in] cs_pin    GPIO number for touch CS.
 * @param[in] irq_pin   GPIO for touch IRQ, or -1 for polling.
 * @param[in] h_res     Horizontal resolution (for mapping).
 * @param[in] v_res     Vertical resolution (for mapping).
 * @return ESP_OK on success, or an error code.
 *
 * @sideeffects Adds SPI device, creates LVGL input device.
 */
esp_err_t touch_init(spi_host_device_t spi_host, int cs_pin,
                     int irq_pin, int h_res, int v_res);

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
