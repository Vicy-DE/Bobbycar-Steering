# Pinout — ESP32-H2 SuperMini

**Last updated:** 2026-03-27
**Board:** ESP32-H2 SuperMini ([AliExpress](https://de.aliexpress.com/item/1005009495310442.html))
**SoC:** ESP32-H2 (RISC-V single-core, 96 MHz, 320 KB SRAM)
**Available GPIOs:** GPIO0–5, GPIO8–14, GPIO22–25 (19 user-accessible on SiP variant)
**Reserved:** GPIO6–7 (not fanned out on SiP), GPIO15–21 (SPI flash), GPIO26–27 (USB-JTAG)

---

## Board Pin Layout

```
              ┌──────────┐
        5V ── │●        ●│ ── GPIO1  (ADC1_CH0)
       GND ── │●        ●│ ── GPIO2  (ADC1_CH1, Strapping)
      3V3  ── │●        ●│ ── GPIO0
     GPIO10── │●        ●│ ── GPIO5  (ADC1_CH4)
     GPIO11── │●        ●│ ── GPIO4  (ADC1_CH3)
     GPIO22── │●        ●│ ── GPIO3  (ADC1_CH2, Strapping)
     GPIO23── │●        ●│ ── GPIO14
     GPIO24── │●        ●│ ── GPIO13
     GPIO25── │●        ●│ ── GPIO12
              │  [USB-C] │
              │  GPIO8   │ (Strapping)
              │  GPIO9   │ (Strapping)
              └──────────┘
```

> **Note:** Exact pin header layout varies by board revision. Verify with your specific board.

---

## Pin Function Table

| GPIO | Direction | Function | Peripheral | Notes |
|------|-----------|----------|------------|-------|
| **GPIO0** | Output | **TWAI TX (CAN)** | TWAI | No ADC on GPIO0 |
| **GPIO1** | Input | **ADC Sensor 1** | ADC1_CH0 | 0–3.3 V analog input |
| **GPIO2** | Input | **ADC Sensor 2** | ADC1_CH1 | Strapping pin — 0–3.3 V analog input |
| **GPIO3** | Input | **TWAI RX (CAN)** | TWAI | Strapping pin, has ADC1_CH2 |
| **GPIO4** | — | **Free** | ADC1_CH3 | Available (has ADC) |
| **GPIO5** | — | **Free** | ADC1_CH4 | Available (has ADC) |
| **GPIO8** | Output | **Onboard WS2812 RGB LED** | RMT | Strapping pin; addressable LED via led_strip driver |
| **GPIO9** | — | **Free** | — | Strapping pin |
| **GPIO10** | Output | **SPI SCK (Display)** | SPI2 | ST7796S clock, 40 MHz |
| **GPIO11** | Output | **SPI CS (Display)** | SPI2 | ST7796S chip select (active low) |
| **GPIO12** | Bidir | **I2C SDA (Touch)** | I2C0 | Reserved for external touch controller |
| **GPIO13** | Bidir | **I2C SCL (Touch)** | I2C0 | Reserved for external touch controller |
| **GPIO14** | — | **Free** | — | Available for future use |
| **GPIO22** | Output | **SPI MOSI (Display)** | SPI2 | ST7796S data |
| **GPIO23** | Output | **Display DC** | GPIO | Data/Command select |
| **GPIO24** | Output | **Display RST** | GPIO | Hardware reset (active low) |
| **GPIO25** | Output | **Display BL** | GPIO | Backlight on/off. Strapping pin |
| GPIO6–7 | — | N/A | — | **Not fanned out** on SiP variant |
| GPIO15–21 | — | SPI Flash | SPI0/1 | **Do not use** |
| GPIO26 | — | USB D− | USB-JTAG | **Reserved** |
| GPIO27 | — | USB D+ | USB-JTAG | **Reserved** |

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
| Channels | CH0 (GPIO1), CH1 (GPIO2) |
| Attenuation | 12 dB (0–3.3 V range) |
| Resolution | 12-bit (0–4095) |
| Mode | Oneshot |

---

## Notes

1. **No Wi-Fi:** ESP32-H2 does not have Wi-Fi. Connectivity via BLE 5 + IEEE 802.15.4 (Zigbee/Thread).
2. **Strapping pins:** GPIO2, GPIO3, GPIO8, GPIO9, GPIO25. GPIO2/GPIO3 used for ADC/CAN — ensure no conflict during boot. Keep GPIO25 high during boot.
3. **ADC GPIO offset:** ADC1_CH0 starts at GPIO1 (not GPIO0 like on ESP32-C3).
4. **SiP flash variant:** GPIO6–7 are not available on boards with SiP flash. GPIO15–21 are used for SPI flash.
5. **Free pins:** GPIO4, GPIO5, GPIO9, GPIO14 are available for expansion (4 free GPIOs). GPIO8 is used by the onboard WS2812 RGB LED.
6. **Lower clock speed:** ESP32-H2 runs at 96 MHz. Display refresh rate may be slightly lower than C3/C5.
