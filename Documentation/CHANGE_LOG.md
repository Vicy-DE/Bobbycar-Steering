# Change Log — Bobbycar-Steering

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
