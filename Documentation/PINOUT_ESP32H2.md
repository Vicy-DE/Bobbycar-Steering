# Pinout — ESP32-H2 SuperMini

**Last updated:** 2026-03-28
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
| **GPIO4** | Input | **SPI MISO / I2C SDA** | SPI2 / I2C | Variant 0: XPT2046 touch data out; Variant 1: FT6236 I2C SDA |
| **GPIO5** | Output | **Blinky LED** | GPIO | Status LED |
| **GPIO8** | Output | **Onboard WS2812 RGB LED** | RMT | Strapping pin; addressable LED via led_strip driver |
| **GPIO9** | Output | **Blinky LED** | GPIO | Strapping pin; status LED |
| **GPIO10** | Output | **SPI SCK (Display+Touch)** | SPI2 | Display clock, 40 MHz |
| **GPIO11** | Output | **SPI CS (Display)** | SPI2 | Display chip select (active low) |
| **GPIO12** | Output | **SPI CS / I2C SCL** | SPI2 / I2C | Variant 0: XPT2046 chip select; Variant 1: FT6236 I2C SCL |
| **GPIO13** | Input | **Touch IRQ / INT** | GPIO | Variant 0: XPT2046 pen IRQ; Variant 1: FT6236 interrupt |
| **GPIO14** | Output | **Blinky LED** | GPIO | Status LED |
| **GPIO22** | Output | **SPI MOSI (Display)** | SPI2 | Display data |
| **GPIO23** | Output | **Display DC** | GPIO | Data/Command select |
| **GPIO24** | Output | **Display RST** | GPIO | Hardware reset (active low) |
| **GPIO25** | Output | **Display BL** | GPIO | Backlight on/off. Strapping pin |
| GPIO6–7 | — | N/A | — | **Not fanned out** on SiP variant |
| GPIO15–21 | — | SPI Flash | SPI0/1 | **Do not use** |
| GPIO26 | — | USB D− | USB-JTAG | **Reserved** |
| GPIO27 | — | USB D+ | USB-JTAG | **Reserved** |

---

## Peripheral Summary

### SPI2 — Display + Touch (Variant 0: ILI9488+XPT2046, Variant 1: ST7796S+FT6236)

| Parameter | Variant 0 (ILI9488) | Variant 1 (ST7796S) |
|-----------|---------------------|---------------------|
| Host | SPI2_HOST | SPI2_HOST |
| Display clock | 40 MHz | 40 MHz |
| Touch interface | SPI (1 MHz, shared bus) | I2C (400 kHz, separate bus) |
| Pixel format | 18-bit RGB666 | 16-bit RGB565 |
| DMA | Auto-allocated | Auto-allocated (zero-copy) |
| MISO | GPIO4 (XPT2046 only) | Not used |
| Display CS | GPIO11 | GPIO11 |
| Touch CS/SCL | GPIO12 (SPI CS) | GPIO12 (I2C SCL) |
| Touch IRQ/INT | GPIO13 | GPIO13 |
| Touch SDA | — | GPIO4 (I2C SDA) |

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
5. **Blinky LEDs:** GPIO5, GPIO9, GPIO14 drive status LEDs. GPIO8 is used by the onboard WS2812 RGB LED.
6. **Lower clock speed:** ESP32-H2 runs at 96 MHz. Display refresh rate may be slightly lower than C3/C5.
7. **Shared SPI bus (Variant 0):** Display and touch share SPI2 (MOSI, MISO, SCK) with separate CS lines. The ESP-IDF SPI driver handles bus arbitration automatically.
8. **Separate I2C bus (Variant 1):** Touch uses I2C0 (GPIO4 SDA, GPIO12 SCL), completely separate from display SPI. No bus contention.
9. **Touch wiring:** See [connection.md](connection.md) for ILI9488 (variant 0) or [connection_st7796s.md](connection_st7796s.md) for ST7796S (variant 1).
