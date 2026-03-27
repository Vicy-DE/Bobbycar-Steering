/**
 * @file display.h
 * @brief Display driver API for ST7796S 3.5" SPI TFT with LVGL.
 */

#pragma once

#include "esp_err.h"
#include "driver/spi_master.h"
#include "lvgl.h"

/**
 * @brief Initialize the ST7796S display and LVGL integration.
 *
 * Configures SPI bus, initializes the ST7796S display controller,
 * sets up LVGL display driver with partial rendering buffers,
 * and turns on the backlight.
 *
 * @param[in] mosi_pin   GPIO number for SPI MOSI.
 * @param[in] sclk_pin   GPIO number for SPI SCLK.
 * @param[in] cs_pin     GPIO number for SPI CS.
 * @param[in] dc_pin     GPIO number for Data/Command select.
 * @param[in] rst_pin    GPIO number for hardware reset.
 * @param[in] bl_pin     GPIO number for backlight control.
 * @param[in] h_res      Horizontal resolution (after rotation).
 * @param[in] v_res      Vertical resolution (after rotation).
 * @param[in] spi_host   SPI host (SPI2_HOST).
 * @param[in] spi_freq   SPI clock frequency in Hz.
 * @return ESP_OK on success, or an error code.
 *
 * @sideeffects Configures SPI2 bus, GPIO pins, allocates DMA buffers,
 *              initializes LVGL library.
 */
esp_err_t display_init(int mosi_pin, int sclk_pin, int cs_pin,
                       int dc_pin, int rst_pin, int bl_pin,
                       int h_res, int v_res,
                       spi_host_device_t spi_host, int spi_freq);

/**
 * @brief Get the LVGL display handle.
 *
 * @return Pointer to the LVGL display, or NULL if not initialized.
 */
lv_display_t *display_get_lvgl_display(void);
