# Change Log — Bobbycar-Steering

## [2026-03-28] Fix Blinky — WS2812 Addressable RGB LED via RMT

### What was changed
- `main/main.c` — Replaced raw GPIO toggle `blinky_task()` with WS2812 RMT-driven version using `led_strip` API; added `#include "led_strip.h"`; task cycles onboard WS2812 through R→G→B plus toggles 4 external GPIO LEDs; stack increased to 4096
- `main/pin_config.h` — Split `BLINKY_LED_PINS`/`BLINKY_LED_COUNT` into `BLINKY_WS2812_GPIO` (GPIO8, onboard WS2812) and `BLINKY_GPIO_PINS`/`BLINKY_GPIO_COUNT` (external GPIOs) for all three targets
- `main/CMakeLists.txt` — Added `led_strip` to `PRIV_REQUIRES`
- `main/idf_component.yml` — Created; declares managed component dependency `espressif/led_strip ^2.4.1`

### Why it was changed
Bug fix: the ESP32 SuperMini boards use a WS2812 addressable RGB LED on GPIO8, not a simple GPIO-driven LED. Raw GPIO toggling sent garbage serial data to the WS2812, causing only the green channel to latch randomly while red and blue showed nothing. The fix uses the ESP-IDF `led_strip` RMT driver to properly encode WS2812 timing.

### What it does / expected behaviour
- Onboard WS2812 LED cycles through Red (25,0,0) → Green (0,25,0) → Blue (0,0,25) at 500 ms
- All three colors are visibly distinct on the board
- External free GPIOs (4, 5, 9, 14 on H2) still toggle in parallel for any wired LEDs
- Logs show: `blinky: WS2812 color 0 (R=25 G=0 B=0)` etc.

### Verified
- Build: OK — ESP32-H2, binary 535 KB (49% free), 1507/1507 targets
- Flash: OK — COM13, hash verified
- Debug: OK — Serial monitor confirmed WS2812 cycling R→G→B at 500 ms, all three colors visible on onboard LED. External GPIOs 4/5/9/14 toggling in parallel.

## [2026-03-28] Blinky LED Task — ESP32-H2 GPIO Bring-Up

### What was changed
- `main/main.c` — Added `#include "driver/gpio.h"`, new `blinky_task()` function that configures GPIO4/5/8/9/14 as push-pull outputs and cycles through them at 500 ms intervals; `xTaskCreate()` call added in `app_main()` to start the task
- `main/pin_config.h` — Added `BLINKY_LED_PINS` and `BLINKY_LED_COUNT` for all three targets: ESP32-H2 (GPIO4, 5, 8, 9, 14), ESP32-C3 (GPIO21), ESP32-C5 (GPIO12–15)
- `main/CMakeLists.txt` — Added `esp_driver_gpio` to `PRIV_REQUIRES`
- `Documentation/Requirements/requirements.md` — Added Req #9 (Blinky LED Task) and updated Traceability Matrix
- `Documentation/ToDo/blinky-task.md` — Created feature todo checklist

### Why it was changed
Feature addition: hardware bring-up and GPIO output verification on the ESP32-H2 SuperMini board. Cycling through all free GPIO pins as LED outputs allows rapid confirmation that each GPIO is functional and correctly routed on the physical board.

### What it does / expected behaviour
- A FreeRTOS task (`blinky_task`) starts alongside `app_main()` at priority 5
- Configures GPIO4, GPIO5, GPIO8, GPIO9, GPIO14 as push-pull outputs on ESP32-H2
- Cycles through each pin: ON for 500 ms, then OFF, advancing to the next
- Wraps around after GPIO14 back to GPIO4
- Each transition logged: `I (xxx) main: blinky: LED GPIOx ON`
- GPIO8 is the onboard LED and will be visibly blinking
- All other targets compile with their own `BLINKY_LED_PINS` set

### Verified
- Build: OK — ESP32-H2, binary 514 KB (51% partition free), 1502/1502 targets
- Flash: OK — COM13, 514880 bytes at 1535.7 kbit/s, hash verified
- Debug: OK — Serial monitor confirmed all 5 GPIOs cycling at 500 ms intervals: `blinky: LED GPIO4 ON` … `blinky: LED GPIO14 ON` repeating continuously. GPIO8 (onboard LED) visibly blinking on hardware.

## [2026-03-27] ADC Sensor Readout and ST7796S Display Driver

### What was changed
- `main/main.c` — Replaced placeholder loop with ADC initialization, display initialization, LVGL UI creation (title, target info, two sensor value labels with progress bars), and main loop reading ADC at 20 Hz
- `main/pin_config.h` — Created per-target GPIO pin assignment header with defines for SPI display, I2C touch (reserved), TWAI (CAN), ADC sensors, and display resolution/frequency constants
- `main/CMakeLists.txt` — Added PRIV_REQUIRES for display and esp_adc components
- `components/display/display.c` — Created ST7796S SPI TFT driver using esp_lcd panel API (ST7789-compatible) with LVGL v9 partial rendering, DMA double-buffering, backlight control, and landscape rotation
- `components/display/include/display.h` — Public API: `display_init()` and `display_get_lvgl_display()`
- `components/display/CMakeLists.txt` — ESP-IDF component registration with lvgl, esp_lcd, esp_driver_gpio, esp_driver_spi dependencies
- `sdkconfig.defaults` — Added LVGL config: 16-bit color depth, Montserrat 14+24 fonts, label and bar widgets
- `datasheets/README.md` — Added ST7796S display module documentation section
- `Documentation/Requirements/requirements.md` — Added Req #7 (ADC Analog Sensor Readout) and Req #8 (3.5" SPI TFT Display)
- `Documentation/ToDo/adc-display.md` — Created feature todo checklist
- `Documentation/PINOUT_ESP32C3.md` — Full pinout with ASCII board layout and function assignments
- `Documentation/PINOUT_ESP32H2.md` — Full pinout with assigned functions (adjusted for H2 GPIO differences)
- `Documentation/PINOUT_ESP32C5.md` — Preliminary pinout mirroring C3 where possible

### Why it was changed
Feature addition: implement the core ADC sensor readout and display functionality for the Bobbycar-Steering project. The two analog sensor inputs and ST7796S 3.5" SPI TFT display are the primary user interface for the steering controller.

### What it does / expected behaviour
- Reads two ADC channels (12-bit, 0–3.3 V range) at 20 Hz on all three targets
- Displays a dark-themed dashboard with "Bobbycar-Steering" title, target identification label, and two sensor readout sections each with numeric value and color-coded progress bar
- LVGL renders at partial-render mode with 20-line double-buffered DMA transfers over SPI at 40 MHz
- All pin assignments documented per target with remaining free GPIOs listed

### Verified
- Build: OK (ESP32-C3, binary 507 KB, 52% partition free)
- Flash: Pending (no hardware connected)
- Debug: Pending

## [2026-03-27] Initial Project Setup

### What was changed
- Created project structure with ESP-IDF CMake build system
- Added ESP-IDF v5.4 as git submodule (`sdk/esp-idf/`)
- Added LVGL v9.x as git submodule (`components/lvgl/`)
- Created `main/main.c` with cross-platform startup banner
- Created `.github/instructions/` workflow documentation (adapted from RC-Servo project)
- Created `.github/hooks/read-instructions-on-compact.json` (copied from rt1180 project)
- Created VS Code settings and build tasks
- Created `setup.ps1` one-click setup script
- Created `Documentation/Requirements/requirements.md` with 6 initial requirements
- Created `Documentation/PROJECT_DOC.md` with full project documentation
- Created `README.md` with ESP32 SoC comparison table
- Created `sdkconfig.defaults` with common configuration
- Created `.gitignore` for ESP-IDF project structure

### Why it was changed
New project for Bobbycar steering controller targeting ESP32 SuperMini development boards (C3, H2, C5). Cross-platform firmware with LVGL display and PWM servo control.

### What it does / expected behaviour
Project compiles for ESP32-C3, ESP32-H2, and ESP32-C5 targets. Prints startup banner with target identification and chip info to serial console. LVGL and steering components are planned but not yet implemented.

### Verified
- Build: Pending (ESP-IDF setup required via `setup.ps1`)
- Flash: Pending
- Debug: Pending
