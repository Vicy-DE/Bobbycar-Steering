/**
 * @file pin_config.h
 * @brief Per-target GPIO pin assignments for Bobbycar-Steering.
 *
 * Defines pin mappings for SPI display (ST7796S), I2C touch,
 * TWAI (CAN), and ADC analog sensors across all three ESP32 targets.
 *
 * @see Documentation/PINOUT_ESP32C3.md
 * @see Documentation/PINOUT_ESP32H2.md
 * @see Documentation/PINOUT_ESP32C5.md
 */

#pragma once

#include "hal/adc_types.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"

/* ------------------------------------------------------------------ */
/*  Display — ST7796S 3.5" SPI TFT (320x480, landscape 480x320)      */
/* ------------------------------------------------------------------ */

#define DISPLAY_H_RES       480
#define DISPLAY_V_RES       320
#define DISPLAY_SPI_HOST    SPI2_HOST
#define DISPLAY_SPI_FREQ_HZ (40 * 1000 * 1000)

/* ------------------------------------------------------------------ */
/*  LVGL draw buffer — 20 lines partial rendering                     */
/* ------------------------------------------------------------------ */

#define LVGL_BUF_LINES      20
#define LVGL_BUF_SIZE       (DISPLAY_H_RES * LVGL_BUF_LINES * sizeof(uint16_t))

/* ================================================================== */
/*  Target-specific pin assignments                                   */
/* ================================================================== */

#if CONFIG_IDF_TARGET_ESP32C3
/* ------- ESP32-C3 SuperMini ------- */
/* Available: GPIO0-10, GPIO20-21    */
/* ADC1: CH0=GPIO0 .. CH4=GPIO4      */
/* USB: GPIO18-19                    */

/* SPI Display */
#define PIN_DISPLAY_MOSI    6
#define PIN_DISPLAY_SCLK    4
#define PIN_DISPLAY_CS      5
#define PIN_DISPLAY_DC      7
#define PIN_DISPLAY_RST     8
#define PIN_DISPLAY_BL      9

/* I2C Touch (reserved) */
#define PIN_TOUCH_SDA       10
#define PIN_TOUCH_SCL       20

/* TWAI (CAN) */
#define PIN_TWAI_TX         2
#define PIN_TWAI_RX         3

/* ADC Sensors */
#define PIN_ADC_SENSOR1     0
#define PIN_ADC_SENSOR2     1
#define ADC_SENSOR1_CHANNEL ADC_CHANNEL_0
#define ADC_SENSOR2_CHANNEL ADC_CHANNEL_1
#define ADC_SENSOR_UNIT     ADC_UNIT_1

/* Free: GPIO21 */

#elif CONFIG_IDF_TARGET_ESP32H2
/* ------- ESP32-H2 SuperMini ------- */
/* Available (SiP): GPIO0-5, GPIO8-14, GPIO22-25 */
/* ADC1: CH0=GPIO1 .. CH4=GPIO5      */
/* USB: GPIO26-27                    */

/* SPI Display */
#define PIN_DISPLAY_MOSI    22
#define PIN_DISPLAY_SCLK    10
#define PIN_DISPLAY_CS      11
#define PIN_DISPLAY_DC      23
#define PIN_DISPLAY_RST     24
#define PIN_DISPLAY_BL      25

/* I2C Touch (reserved) */
#define PIN_TOUCH_SDA       12
#define PIN_TOUCH_SCL       13

/* TWAI (CAN) */
#define PIN_TWAI_TX         0
#define PIN_TWAI_RX         3

/* ADC Sensors */
#define PIN_ADC_SENSOR1     1
#define PIN_ADC_SENSOR2     2
#define ADC_SENSOR1_CHANNEL ADC_CHANNEL_0
#define ADC_SENSOR2_CHANNEL ADC_CHANNEL_1
#define ADC_SENSOR_UNIT     ADC_UNIT_1

/* Free: GPIO4, GPIO5, GPIO8, GPIO9, GPIO14 */

#elif CONFIG_IDF_TARGET_ESP32C5
/* ------- ESP32-C5 SuperMini ------- */
/* 29 GPIOs total, many available    */

/* SPI Display */
#define PIN_DISPLAY_MOSI    6
#define PIN_DISPLAY_SCLK    4
#define PIN_DISPLAY_CS      5
#define PIN_DISPLAY_DC      7
#define PIN_DISPLAY_RST     8
#define PIN_DISPLAY_BL      9

/* I2C Touch (reserved) */
#define PIN_TOUCH_SDA       10
#define PIN_TOUCH_SCL       11

/* TWAI (CAN) */
#define PIN_TWAI_TX         2
#define PIN_TWAI_RX         3

/* ADC Sensors */
#define PIN_ADC_SENSOR1     0
#define PIN_ADC_SENSOR2     1
#define ADC_SENSOR1_CHANNEL ADC_CHANNEL_0
#define ADC_SENSOR2_CHANNEL ADC_CHANNEL_1
#define ADC_SENSOR_UNIT     ADC_UNIT_1

/* Free: GPIO12+ (many available) */

#else
#error "Unsupported IDF target. Use esp32c3, esp32h2, or esp32c5."
#endif
