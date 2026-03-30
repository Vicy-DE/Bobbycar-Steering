# Connection Guide — ILI9488 4.0" SPI TFT + Touch → ESP32-H2 SuperMini

**Display:** [AliExpress — 4.0" ILI9488 SPI TFT with XPT2046 Touch](https://a.aliexpress.com/_EIdLXLa)
**Board:** ESP32-H2 SuperMini

---

## Display Module

4.0" TFT LCD, ILI9488 driver IC, 480×320 pixels, SPI interface.
Resistive touch via XPT2046 on the same SPI bus (separate CS).
14-pin header (active-low CS, active-low RST, active-high backlight).

```
┌──────────────────────────────────────────────┐
│              ILI9488 4.0" TFT                │
│              480 × 320 + Touch               │
│                                              │
│   ┌──────────────────────────────────────┐   │
│   │                                      │   │
│   │          Display Area                │   │
│   │          480 × 320 px                │   │
│   │         (landscape mode)             │   │
│   │                                      │   │
│   └──────────────────────────────────────┘   │
│                                              │
│ VCC GND CS RST DC SDI SCK LED SDO            │
│  1   2  3   4  5  6   7   8   9             │
│                                              │
│ T_CLK T_CS T_DIN T_DO T_IRQ                 │
│  10    11   12    13   14                    │
└──────────────────────────────────────────────┘
```

> **Pin order may vary by board revision.** Check your specific module's silkscreen markings.

---

## ESP32-H2 SuperMini Board

```
              ┌──────────┐
        5V ── │●        ●│ ── GPIO1  (ADC Sensor 1)
       GND ── │●        ●│ ── GPIO2  (ADC Sensor 2)
      3V3  ── │●        ●│ ── GPIO0  (TWAI TX)
  [SCLK] 10── │●        ●│ ── GPIO5  (Blinky)
    [CS] 11── │●        ●│ ── GPIO4  [MISO]
  [MOSI] 22── │●        ●│ ── GPIO3  (TWAI RX)
    [DC] 23── │●        ●│ ── GPIO14 (Blinky)
   [RST] 24── │●        ●│ ── GPIO13 [T_IRQ]
    [BL] 25── │●        ●│ ── GPIO12 [T_CS]
              │  [USB-C] │
              │  GPIO8   │ (WS2812)
              │  GPIO9   │ (Blinky)
              └──────────┘
```

---

## Wiring Table

| # | Display Pin | Function | ESP32-H2 | Wire Color (suggested) |
|---|-------------|----------|----------|----------------------|
| 1 | VCC | Power 3.3V | 3V3 | Red |
| 2 | GND | Ground | GND | Black |
| 3 | CS | Display chip select | GPIO11 | Yellow |
| 4 | RST | Display reset | GPIO24 | White |
| 5 | DC | Data/Command | GPIO23 | Orange |
| 6 | SDI | SPI MOSI (shared) | GPIO22 | Blue |
| 7 | SCK | SPI Clock (shared) | GPIO10 | Green |
| 8 | LED | Backlight | GPIO25 | Red |
| 9 | SDO | Display MISO | — | **Leave NC** |
| 10 | T_CLK | Touch clock | GPIO10 | Green (same as SCK) |
| 11 | T_CS | Touch chip select | GPIO12 | Purple |
| 12 | T_DIN | Touch MOSI | GPIO22 | Blue (same as SDI) |
| 13 | T_DO | Touch MISO | GPIO4 | Grey |
| 14 | T_IRQ | Touch interrupt | GPIO13 | Brown |

---

## Wiring Diagram

```
   ESP32-H2 SuperMini              ILI9488 Display Module
  ┌──────────────┐                ┌──────────────────────┐
  │              │                │                      │
  │   3V3  ──────┼────────────────┤ VCC (pin 1)          │
  │   GND  ──────┼────────────────┤ GND (pin 2)          │
  │              │                │                      │
  │   GPIO22 ────┼──┬─────────────┤ SDI (pin 6)  MOSI    │
  │              │  └─────────────┤ T_DIN (pin 12)       │
  │              │                │                      │
  │   GPIO10 ────┼──┬─────────────┤ SCK (pin 7)  Clock   │
  │              │  └─────────────┤ T_CLK (pin 10)       │
  │              │                │                      │
  │   GPIO4  ────┼────────────────┤ T_DO (pin 13) MISO   │
  │              │                │                      │
  │   GPIO11 ────┼────────────────┤ CS (pin 3)   Disp CS │
  │   GPIO23 ────┼────────────────┤ DC (pin 5)           │
  │   GPIO24 ────┼────────────────┤ RST (pin 4)          │
  │   GPIO25 ────┼────────────────┤ LED (pin 8)  Backlt  │
  │              │                │                      │
  │   GPIO12 ────┼────────────────┤ T_CS (pin 11) Tch CS │
  │   GPIO13 ────┼────────────────┤ T_IRQ (pin 14)       │
  │              │                │                      │
  │         NC   │           NC ──┤ SDO (pin 9)          │
  │              │                │                      │
  │  [USB-C]     │                │                      │
  └──────────────┘                └──────────────────────┘
```

---

## Important Notes

1. **3.3V only** — The ILI9488 module runs on 3.3V. Do NOT connect to 5V.

2. **Shared SPI lines** — MOSI (SDI/T_DIN) and SCK (SCK/T_CLK) are shared between display and touch. If your module has separate pins for these, connect them to the same ESP32 GPIO. Some modules require soldering jumper pads to bridge these connections.

3. **Leave SDO unconnected** — The display's MISO (SDO, pin 9) is not needed. Only the touch MISO (T_DO, pin 13) is used.

4. **Touch IRQ is optional** — T_IRQ (pin 14) can be left unconnected. The driver uses polling mode. Connecting it allows future interrupt-driven touch detection.

5. **Touch calibration** — The XPT2046 raw ADC values (0–4095) are mapped to screen coordinates. If touch coordinates are offset, swap X/Y or adjust `TOUCH_X_MIN`/`TOUCH_X_MAX`/`TOUCH_Y_MIN`/`TOUCH_Y_MAX` in `components/display/display.c`.

6. **SPI frequency** — Default is 40 MHz. If the display shows garbled graphics, lower `DISPLAY_SPI_FREQ_HZ` in `pin_config.h` to 20 MHz or 10 MHz.
