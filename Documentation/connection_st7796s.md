# Connection Guide — ST7796S 4.0" SPI TFT + FT6236 Touch → ESP32-H2 SuperMini

**Display:** [AliExpress — 4.0" ST7796S SPI TFT with FT6236 Capacitive Touch](https://de.aliexpress.com/item/1005007609041421.html)
**Board:** ESP32-H2 SuperMini
**Compile:** `DISPLAY_VARIANT=1` (set in root CMakeLists.txt)

---

## Display Module

4.0" TFT LCD, ST7796S driver IC, 480×320 pixels, SPI interface.
Capacitive touch via FT6236 on a separate I2C bus (address 0x38).
12-pin header (active-low CS, active-low RST, active-high backlight).

```
┌──────────────────────────────────────────────┐
│            ST7796S 4.0" TFT                  │
│         480 × 320 + Cap. Touch               │
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
│ T_SCL T_SDA T_INT                            │
│  10    11    12                              │
└──────────────────────────────────────────────┘
```

> **Pin order may vary by board revision.** Check your specific module's silkscreen markings.
> The "NO TOUCH" module variant omits pins 10–12.

---

## ESP32-H2 SuperMini Board

```
              ┌──────────┐
        5V ── │●        ●│ ── GPIO1  (ADC Sensor 1)
       GND ── │●        ●│ ── GPIO2  (ADC Sensor 2)
      3V3  ── │●        ●│ ── GPIO0  (TWAI TX)
  [SCLK] 10── │●        ●│ ── GPIO5  (Blinky)
    [CS] 11── │●        ●│ ── GPIO4  [T_SDA]
  [MOSI] 22── │●        ●│ ── GPIO3  (TWAI RX)
    [DC] 23── │●        ●│ ── GPIO14 (Blinky)
   [RST] 24── │●        ●│ ── GPIO13 [T_INT]
    [BL] 25── │●        ●│ ── GPIO12 [T_SCL]
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
| 6 | SDI | SPI MOSI | GPIO22 | Blue |
| 7 | SCK | SPI Clock | GPIO10 | Green |
| 8 | LED | Backlight | GPIO25 | Red |
| 9 | SDO | Display MISO | — | **Leave NC** (not needed) |
| 10 | T_SCL | Touch I2C clock | GPIO12 | Purple |
| 11 | T_SDA | Touch I2C data | GPIO4 | Grey |
| 12 | T_INT | Touch interrupt | GPIO13 | Brown |

---

## Wiring Diagram

```
   ESP32-H2 SuperMini              ST7796S Display Module
  ┌──────────────┐                ┌──────────────────────┐
  │              │                │                      │
  │   3V3  ──────┼────────────────┤ VCC (pin 1)          │
  │   GND  ──────┼────────────────┤ GND (pin 2)          │
  │              │                │                      │
  │              │ Display SPI    │                      │
  │   GPIO22 ────┼────────────────┤ SDI (pin 6)  MOSI    │
  │   GPIO10 ────┼────────────────┤ SCK (pin 7)  Clock   │
  │   GPIO11 ────┼────────────────┤ CS (pin 3)   Disp CS │
  │   GPIO23 ────┼────────────────┤ DC (pin 5)           │
  │   GPIO24 ────┼────────────────┤ RST (pin 4)          │
  │   GPIO25 ────┼────────────────┤ LED (pin 8)  Backlt  │
  │              │                │                      │
  │              │ Touch I2C      │                      │
  │   GPIO4  ────┼────────────────┤ T_SDA (pin 11)       │
  │   GPIO12 ────┼────────────────┤ T_SCL (pin 10)       │
  │   GPIO13 ────┼────────────────┤ T_INT (pin 12)       │
  │              │                │                      │
  │         NC   │           NC ──┤ SDO (pin 9)          │
  │              │                │                      │
  │  [USB-C]     │                │                      │
  └──────────────┘                └──────────────────────┘
```

---

## Comparison: ILI9488 (Variant 0) vs. ST7796S (Variant 1)

| Feature | ILI9488 (Variant 0) | ST7796S (Variant 1) |
|---------|--------------------|--------------------|
| Display IC | ILI9488 | ST7796S |
| SPI pixel format | 18-bit RGB666 | **16-bit RGB565** |
| Touch IC | XPT2046 (resistive) | **FT6236 (capacitive)** |
| Touch interface | SPI (shared bus) | **I2C (separate bus)** |
| MISO pin needed | Yes (GPIO4 for touch) | No |
| SPI bytes/pixel | 3 | **2 (33% faster)** |
| Conversion buffer | 8,640 bytes | **None (zero-copy DMA)** |
| LVGL buffer lines | 6 | **10 (66% larger)** |
| Total display RAM | 20,160 bytes | **19,200 bytes (saves 960)** |
| Touch calibration | Manual (raw ADC → pixel) | **Factory-calibrated** |
| Total wires | 11 (9 GPIO + VCC + GND) | 11 (9 GPIO + VCC + GND) |

### GPIO Reuse

Pins GPIO4, GPIO12, GPIO13 are reused between variants with different protocols:

| GPIO | Variant 0 (ILI9488) | Variant 1 (ST7796S) |
|------|---------------------|---------------------|
| GPIO4 | SPI MISO (T_DO) | I2C SDA (T_SDA) |
| GPIO12 | SPI CS (T_CS) | I2C SCL (T_SCL) |
| GPIO13 | GPIO (T_IRQ) | GPIO (T_INT) |

---

## Important Notes

1. **3.3V only** — The ST7796S module runs on 3.3V. Do NOT connect to 5V.

2. **No shared bus** — Unlike the ILI9488 variant, touch uses I2C (GPIO4/12) which is completely separate from the display SPI bus (GPIO10/22). This means touch reads never block display DMA transfers.

3. **Leave SDO unconnected** — The display MISO (SDO, pin 9) is not needed for write-only operation.

4. **I2C pull-ups** — The ESP32-H2 internal pull-ups are enabled by the driver. For long wires (>10 cm), add external 4.7 kΩ pull-ups on SDA and SCL to 3.3V.

5. **FT6236 address** — Fixed at 0x38. No address configuration jumpers.

6. **Touch orientation** — The FT6236 returns coordinates in the panel's native portrait orientation. The driver swaps X/Y for landscape mode. If touch coordinates appear mirrored, adjust `FT_SWAP_XY`, `FT_INVERT_X`, `FT_INVERT_Y` in `display.c`.
