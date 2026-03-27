# Pinout — ESP32-C3 SuperMini

**Last updated:** 2026-03-27
**Board:** ESP32-C3 SuperMini ([AliExpress](https://de.aliexpress.com/item/1005009495310442.html))
**SoC:** ESP32-C3 (RISC-V single-core, 160 MHz, 400 KB SRAM)
**Available GPIOs:** GPIO0–GPIO10, GPIO20, GPIO21 (13 user-accessible)
**Reserved:** GPIO12–17 (SPI flash), GPIO18–19 (USB-JTAG)

---

## Board Pin Layout

```
              ┌──────────┐
        5V ── │●        ●│ ── GPIO0  (ADC1_CH0)
       GND ── │●        ●│ ── GPIO1  (ADC1_CH1)
      3V3  ── │●        ●│ ── GPIO2  (Strapping)
     GPIO4 ── │●        ●│ ── GPIO3
     GPIO5 ── │●        ●│ ── GPIO10
     GPIO6 ── │●        ●│ ── GPIO20
     GPIO7 ── │●        ●│ ── GPIO21
     GPIO8 ── │●        ●│ ── GPIO9  (Strapping)
              │  [USB-C] │
              └──────────┘
```

---

## Pin Function Table

| GPIO | Direction | Function | Peripheral | Notes |
|------|-----------|----------|------------|-------|
| **GPIO0** | Input | **ADC Sensor 1** | ADC1_CH0 | 0–3.3 V analog input |
| **GPIO1** | Input | **ADC Sensor 2** | ADC1_CH1 | 0–3.3 V analog input |
| **GPIO2** | Output | **TWAI TX (CAN)** | TWAI | Strapping pin — external pull-up recommended |
| **GPIO3** | Input | **TWAI RX (CAN)** | TWAI | CAN bus receive |
| **GPIO4** | Output | **SPI SCK (Display)** | SPI2 | ST7796S clock, 40 MHz |
| **GPIO5** | Output | **SPI CS (Display)** | SPI2 | ST7796S chip select (active low) |
| **GPIO6** | Output | **SPI MOSI (Display)** | SPI2 | ST7796S data |
| **GPIO7** | Output | **Display DC** | GPIO | Data/Command select |
| **GPIO8** | Output | **Display RST** | GPIO | Hardware reset (active low). Strapping pin |
| **GPIO9** | Output | **Display BL** | GPIO | Backlight on/off. Strapping pin |
| **GPIO10** | Bidir | **I2C SDA (Touch)** | I2C0 | Reserved for external touch controller |
| **GPIO20** | Bidir | **I2C SCL (Touch)** | I2C0 | Reserved for external touch controller |
| **GPIO21** | — | **Free** | — | Available for future use |
| GPIO12–17 | — | SPI Flash | SPI0/1 | **Do not use** |
| GPIO18 | — | USB D− | USB-JTAG | **Reserved** |
| GPIO19 | — | USB D+ | USB-JTAG | **Reserved** |

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

1. **Strapping pins:** GPIO2, GPIO8, GPIO9 affect boot mode. GPIO2 must be floating or high during boot for normal SPI boot. GPIO8 should be high, GPIO9 pulled high for normal boot.
2. **CAN bus:** Requires external CAN transceiver. TWAI TX/RX connect to transceiver TXD/RXD.
3. **Display backlight:** GPIO9 directly drives BL pin. For PWM dimming, use LEDC peripheral on GPIO9.
4. **ADC accuracy:** Use `adc_cali` driver for voltage-calibrated readings.
5. **Pin count:** All 13 user-accessible GPIOs are assigned. GPIO21 is the only free pin.
