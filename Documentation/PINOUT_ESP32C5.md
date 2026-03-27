# Pinout — ESP32-C5 SuperMini

**Last updated:** 2026-03-27
**Board:** ESP32-C5 SuperMini ([AliExpress](https://de.aliexpress.com/item/1005009495310442.html))
**SoC:** ESP32-C5 (RISC-V single-core, 240 MHz, 384 KB SRAM, external PSRAM support)
**Available GPIOs:** Up to 29 programmable GPIOs
**Reserved:** USB-Serial-JTAG pins, SPI flash pins

> **Note:** ESP32-C5 SuperMini boards are newer. Pin header layout may vary between manufacturers. Verify GPIO numbering with your specific board revision.

---

## Board Pin Layout

```
              ┌──────────┐
        5V ── │●        ●│ ── GPIO0  (ADC)
       GND ── │●        ●│ ── GPIO1  (ADC)
      3V3  ── │●        ●│ ── GPIO2
     GPIO4 ── │●        ●│ ── GPIO3
     GPIO5 ── │●        ●│ ── GPIO10
     GPIO6 ── │●        ●│ ── GPIO11
     GPIO7 ── │●        ●│ ── GPIO12
     GPIO8 ── │●        ●│ ── GPIO9
              │  [USB-C] │
              └──────────┘
```

> **Preliminary:** Exact pin layout TBD pending final board availability. Layout shown is estimated based on C3 SuperMini form factor.

---

## Pin Function Table

| GPIO | Direction | Function | Peripheral | Notes |
|------|-----------|----------|------------|-------|
| **GPIO0** | Input | **ADC Sensor 1** | ADC1_CH0 | 0–3.3 V analog input |
| **GPIO1** | Input | **ADC Sensor 2** | ADC1_CH1 | 0–3.3 V analog input |
| **GPIO2** | Output | **TWAI TX (CAN)** | TWAI | CAN bus transmit |
| **GPIO3** | Input | **TWAI RX (CAN)** | TWAI | CAN bus receive |
| **GPIO4** | Output | **SPI SCK (Display)** | SPI2 | ST7796S clock, 40 MHz |
| **GPIO5** | Output | **SPI CS (Display)** | SPI2 | ST7796S chip select (active low) |
| **GPIO6** | Output | **SPI MOSI (Display)** | SPI2 | ST7796S data |
| **GPIO7** | Output | **Display DC** | GPIO | Data/Command select |
| **GPIO8** | Output | **Display RST** | GPIO | Hardware reset (active low) |
| **GPIO9** | Output | **Display BL** | GPIO | Backlight on/off |
| **GPIO10** | Bidir | **I2C SDA (Touch)** | I2C0 | Reserved for external touch controller |
| **GPIO11** | Bidir | **I2C SCL (Touch)** | I2C0 | Reserved for external touch controller |
| **GPIO12+** | — | **Free** | — | Additional GPIOs available for expansion |
| USB pins | — | USB-Serial-JTAG | USB | **Reserved** |
| Flash pins | — | SPI Flash | SPI0/1 | **Do not use** |

---

## Peripheral Summary

### SPI2 — Display (ST7796S 3.5" 320×480)

| Parameter | Value |
|-----------|-------|
| Host | SPI2_HOST |
| Clock | 40 MHz |
| Mode | SPI Mode 0 (CPOL=0, CPHA=0) |
| Data width | 8-bit commands, 16-bit pixel data (RGB565) |
| DMA | Auto-allocated |
| MISO | Not connected (write-only) |

### I2C0 — Touch Controller (Reserved)

| Parameter | Value |
|-----------|-------|
| Clock | 400 kHz (Fast Mode) |
| Pull-ups | External 4.7 kΩ required |
| Status | Reserved — no touch on current display module |

### TWAI — CAN Bus

| Parameter | Value |
|-----------|-------|
| Mode | Normal |
| Baud rate | Configurable (default 500 kbps) |
| Transceiver | External CAN transceiver required (e.g., SN65HVD230) |

### ADC1 — Analog Sensors

| Parameter | Value |
|-----------|-------|
| Unit | ADC1 |
| Attenuation | 12 dB (0–3.3 V range) |
| Resolution | 12-bit (0–4095) |
| Mode | Oneshot |

---

## Notes

1. **Wi-Fi 6 dual-band:** ESP32-C5 supports 2.4 GHz and 5 GHz Wi-Fi 6. Best wireless performance of the three targets.
2. **Highest clock:** 240 MHz provides the best display refresh rate and processing power.
3. **PSRAM support:** External PSRAM can be added for larger LVGL frame buffers or complex UIs.
4. **Pin assignments match C3:** Where possible, pin assignments mirror ESP32-C3 for easier cross-platform development.
5. **Preliminary:** ESP32-C5 support in ESP-IDF v5.4 may be in preview. Some features could change in future IDF releases.
6. **Many free pins:** ESP32-C5 has significantly more GPIOs than C3/H2, providing ample expansion room.
