/**
 * @file pin_config.h
 * @brief GPIO pin assignments for Bobbycar-Steering (ESP32-H2 only).
 *
 * Supports two display hardware variants selected at compile time
 * via DISPLAY_VARIANT (set in the project root CMakeLists.txt):
 *   0 — ILI9488 4.0" (18-bit RGB666) + XPT2046 resistive SPI touch
 *   1 — ST7796S 4.0" (16-bit RGB565) + FT6236 capacitive I2C touch
 *
 * @see Documentation/PINOUT_ESP32H2.md
 * @see Documentation/connection.md
 * @see Documentation/connection_st7796s.md
 */

#pragma once

#include "hal/adc_types.h"
#include "driver/spi_master.h"
#include "esp_adc/adc_oneshot.h"

#if !CONFIG_IDF_TARGET_ESP32H2
#error "This project targets ESP32-H2 only."
#endif

/* ------------------------------------------------------------------ */
/*  Display variant — set in project root CMakeLists.txt              */
/*    0 = ILI9488 + XPT2046  (SPI display + SPI resistive touch)     */
/*    1 = ST7796S + FT6236   (SPI display + I2C capacitive touch)    */
/* ------------------------------------------------------------------ */

#ifndef DISPLAY_VARIANT
#define DISPLAY_VARIANT 0
#endif

/* ------------------------------------------------------------------ */
/*  Display — 4.0" SPI TFT (320x480, landscape 480x320)              */
/* ------------------------------------------------------------------ */

#define DISPLAY_H_RES       480
#define DISPLAY_V_RES       320
#define DISPLAY_SPI_HOST    SPI2_HOST
#define DISPLAY_SPI_FREQ_HZ (40 * 1000 * 1000)

/* ================================================================== */
/*  ESP32-H2 SuperMini Pin Assignments                                */
/*  Available (SiP): GPIO0-5, GPIO8-14, GPIO22-25                    */
/*  ADC1: CH0=GPIO1 .. CH4=GPIO5                                     */
/*  USB-JTAG: GPIO26-27                                               */
/* ================================================================== */

/* ---- SPI Display (shared by both variants) ---- */
#define PIN_DISPLAY_MOSI    22
#define PIN_DISPLAY_SCLK    10
#define PIN_DISPLAY_CS      11
#define PIN_DISPLAY_DC      23
#define PIN_DISPLAY_RST     24
#define PIN_DISPLAY_BL      25

/* ---- Touch (variant-specific) ---- */
#if DISPLAY_VARIANT == 0
/*  ILI9488 + XPT2046: SPI touch on same bus, needs MISO */
#define PIN_DISPLAY_MISO    4
#define PIN_TOUCH_CS        12
#define PIN_TOUCH_IRQ       13
#elif DISPLAY_VARIANT == 1
/*  ST7796S + FT6236: I2C capacitive touch, no SPI MISO needed */
#define PIN_TOUCH_SDA       4
#define PIN_TOUCH_SCL       12
#define PIN_TOUCH_INT       13
#else
#error "DISPLAY_VARIANT must be 0 (ILI9488+XPT2046) or 1 (ST7796S+FT6236)."
#endif

/* ---- TWAI (CAN) ---- */
#define PIN_TWAI_TX         0
#define PIN_TWAI_RX         3

/* ---- ADC Sensors ---- */
#define PIN_ADC_SENSOR1     1
#define PIN_ADC_SENSOR2     2
#define ADC_SENSOR1_CHANNEL ADC_CHANNEL_0
#define ADC_SENSOR2_CHANNEL ADC_CHANNEL_1
#define ADC_SENSOR_UNIT     ADC_UNIT_1

/* ---- Blinky — onboard WS2812 RGB LED + free GPIO outputs ---- */
#define BLINKY_WS2812_GPIO  8
#define BLINKY_GPIO_PINS    { 5, 9, 14 }
#define BLINKY_GPIO_COUNT   3
