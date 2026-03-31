# Change Log — Bobbycar-Steering

## [2026-03-31] ST7796S + FT6236 display variant, H2-only, RAM optimization

### What was changed
- `main/pin_config.h` — Removed all ESP32-C3/C5 sections; added `#if !CONFIG_IDF_TARGET_ESP32H2` compile error guard; added `DISPLAY_VARIANT` compile-time switch (0=ILI9488+XPT2046, 1=ST7796S+FT6236); GPIO4/12/13 repurposed per variant (SPI MISO/T_CS/T_IRQ vs I2C SDA/SCL/INT)
- `CMakeLists.txt` (root) — Added `add_compile_definitions(DISPLAY_VARIANT=0)` after `project()` for compile-time display selection
- `components/display/CMakeLists.txt` — Added `esp_driver_i2c` to PRIV_REQUIRES for FT6236 I2C touch
- `components/display/include/display.h` — Rewritten with `#if DISPLAY_VARIANT` conditional API; variant 0: `display_init()` with MISO + `touch_init()` with SPI host/CS/IRQ; variant 1: `display_init()` without MISO + `touch_init()` with I2C SDA/SCL/INT
- `components/display/display.c` — Major rewrite: variant 0 retains ILI9488 18-bit RGB666 + XPT2046 (LVGL_BUF_LINES=6, 20160 bytes); variant 1 adds ST7796S 16-bit RGB565 zero-copy DMA + FT6236 I2C capacitive touch (LVGL_BUF_LINES=10, 19200 bytes); `#if DISPLAY_VARIANT` blocks throughout; added FT6236 I2C read via ESP-IDF v5.4 `i2c_master` API
- `main/main.c` — Removed C3/C5 target `#if` blocks; simplified to H2-only; added `#if DISPLAY_VARIANT` for display_init/touch_init parameter selection; version bumped to v0.8
- `Documentation/connection_st7796s.md` — Created wiring guide with ASCII art diagrams for ST7796S+FT6236 → ESP32-H2 SuperMini; includes comparison table (ILI9488 vs ST7796S)

### Why it was changed
Added ST7796S 4.0" SPI TFT with FT6236 capacitive touch as a second display option. ST7796S supports native 16-bit RGB565 over SPI (no conversion buffer needed), and the capacitive touch uses I2C on a separate bus (no SPI contention). Removed ESP32-C3 and ESP32-C5 targets to simplify project (only H2 SuperMini boards are used).

### What it does / expected behaviour
- DISPLAY_VARIANT=0: ILI9488 + XPT2046 (existing hardware) — no behaviour change
- DISPLAY_VARIANT=1: ST7796S + FT6236 — zero-copy DMA (2 bytes/pixel vs 3), no conversion buffer, 10-line LVGL bufs (vs 6), total 19200 bytes (saves 960 vs variant 0), capacitive touch on I2C (addr 0x38, factory-calibrated)
- Same 9 GPIOs used by both variants (4/12/13 repurposed SPI↔I2C)
- Project now H2-only — compile error if target is not ESP32-H2

### Verified
- Build: OK — ESP32-H2, `idf.py build` (1756/1756), DISPLAY_VARIANT=0
- Flash: OK — COM14, hash verified
- Debug: OK — Serial output: "Bobbycar-Steering v0.8", "Display variant: 0", "ILI9488 display (480x320)", "6-line bufs, 18-bit RGB666, total 20160 bytes", "XPT2046 touch (CS=12, IRQ=13)", Bluepad32+BLE running, 73.9 KB free heap

## [2026-03-30] ILI9488 Display Port + XPT2046 Touch Input

### What was changed
- `main/pin_config.h` — Replaced ST7796S pin definitions with ILI9488; added `PIN_DISPLAY_MISO`, `PIN_TOUCH_CS`, `PIN_TOUCH_IRQ` for all 3 targets (H2/C3/C5); removed unused `LVGL_BUF_LINES`/`LVGL_BUF_SIZE` defines; reassigned blinky GPIOs
- `components/display/display.c` — Major rewrite: ILI9488 18-bit RGB666 mode via `bits_per_pixel=18`; added RGB565→RGB666 conversion in flush callback; added XPT2046 SPI touch driver (4-sample averaging, calibrated coordinate mapping); reduced `LVGL_BUF_LINES` to 6; added `touch_init()`, `touch_get_indev()`, `touch_get_last_point()`
- `components/display/include/display.h` — Updated `display_init()` signature with `miso_pin` parameter; added `touch_init()`, `touch_get_indev()`, `touch_get_last_point()` declarations
- `main/main.c` — Updated `display_init()` call with `PIN_DISPLAY_MISO`; added `touch_init()` call; added touch coordinates label (`s_label_touch`); added demo button with press counter (`btn_event_cb`)
- `sdkconfig.defaults` — Added `CONFIG_LV_USE_BUTTON=y`
- `Documentation/connection.md` — Created wiring guide with ASCII art diagrams for ILI9488 module → ESP32-H2 SuperMini
- `Documentation/PINOUT_ESP32H2.md` — Updated GPIO4/5/9/10/11/12/13/14/22 assignments; replaced I2C0 touch section with SPI2 shared bus (display+touch); added shared SPI bus note

### Why it was changed
Hardware change: porting from ST7796S 3.5" display to ILI9488 4.0" 480×320 SPI TFT with XPT2046 resistive touch controller. The ILI9488 requires 18-bit RGB666 over SPI (not 16-bit RGB565), requiring a conversion buffer in the flush callback. Touch input enables interactive UI elements.

### What it does / expected behaviour
- ILI9488 display initializes in 18-bit RGB666 mode (landscape 480×320)
- XPT2046 resistive touch controller shares SPI2 bus with display (separate CS)
- LVGL renders RGB565 internally; flush callback converts to RGB666 (3 bytes/pixel) before DMA transfer
- Touch coordinates displayed on screen; demo button increments press counter on touch
- Memory budget: 2×5,760 (LVGL bufs) + 8,640 (conv buf) = 20,160 bytes total display memory
- Boot heap: 73,904 bytes — ample for BLE controller

### Verified
- Build: OK — ESP32-H2, `idf.py build` (1756/1756)
- Flash: OK — COM14, hash verified
- Debug: OK — Serial output confirms: "ILI9488 display (480x320)", "6-line bufs, 18-bit RGB666", "XPT2046 touch (CS=12, IRQ=13)", BLE running, 73.9 KB free heap

## [2026-03-29] Power Management, BLE Console, INI Config, C++ Conversion, LVGL Buffer Fix

### What was changed
- `main/power_mgmt.h` — Created power management API header
- `main/power_mgmt.c` — Created DFS implementation: `esp_pm_configure()` with per-target min/max frequencies (H2: 32–96 MHz, C3: 80–160 MHz, C5: 80–240 MHz), light sleep disabled (incompatible with BTstack VHCI)
- `main/ble_console.h` — Created BLE console API: `ble_console_init()`, `console_printf()` dual-output function
- `main/ble_console.c` — Created Nordic UART Service (NUS) implementation via BTstack GATT server: TX ring buffer (1 KB), RX line buffer (128 bytes), MTU negotiation, incoming data dispatched through `console_exec()`
- `main/bp32_config.h` — Created Bluepad32 INI configuration API: general config, button mapping, known device structs
- `main/bp32_config.c` — Created INI-based config management: `bluepad32.ini` (general), `buttonmap.ini` (controls), `devices/*.ini` (known BT addresses with FIFO eviction at 8 devices), default creation on first boot
- `main/ini_parser.h` — Created minimal INI parser API
- `main/ini_parser.c` — Created INI file parser: `[sections]`, `key=value`, `#`/`;` comments, `ini_parse_file()` and `ini_write_value()` with in-place update
- `main/motor.cpp` — Converted from `motor.c` to C++: Motor class with private members, const getters, MotorController singleton, `extern "C"` wrappers
- `main/steering_algo.cpp` — Converted from `steering_algo.c` to C++: uses `<cmath>`, `static_cast<>`, C-linkage API preserved
- `main/my_platform.c` — Added `ble_console_init()` and `bp32_config_load()` calls in `on_init_complete` callback
- `main/console.c` — Updated "Unknown command" message to use `console_printf()` for BLE output
- `main/console_cmds.c` — Added `#include "ble_console.h"`, `#define printf console_printf` for BLE output; added `cfgload`/`cfgsave` commands; fixed `cmd_ver()` double-escaped `\\r\\n` → `\r\n`; version bumped to v0.7
- `main/console_cmds_fs.c` — Added `#include "ble_console.h"`, `#define printf console_printf` for BLE output
- `main/main.c` — Added `power_mgmt_init()` call; version bumped to v0.7
- `main/CMakeLists.txt` — Added power_mgmt.c, ini_parser.c, bp32_config.c, ble_console.c to SRCS; renamed motor.c→motor.cpp, steering_algo.c→steering_algo.cpp
- `components/display/display.c` — Reduced `LVGL_BUF_LINES` from 20 to 10 (saves 19,200 bytes of heap for BLE controller init)
- `components/bluepad32/src/components/bluepad32/bt/uni_bt_service.gatt` — Added NUS service import
- `components/bluepad32/.../uni_bt_service.gatt.h` — Regenerated with NUS handles 0x0023–0x0028
- `sdkconfig.defaults` — Added `CONFIG_PM_ENABLE=y`, `CONFIG_FREERTOS_USE_TICKLESS_IDLE=y`, `CONFIG_BT_LE_SLEEP_ENABLE=y`
- `Documentation/PROJECT_DOC.md` — Updated to v0.7: added CAN bus architecture section, new modules, boot flow, known limitations
- `Documentation/Requirements/requirements.md` — Added requirements #15-21 (C++ objects, PM, LittleFS test, INI config, BLE console, Android app, CAN bus)

### Why it was changed
Feature additions: power management reduces idle power via DFS (CPU scales to 32 MHz when idle). BLE console enables wireless terminal access via Nordic UART Service. INI config allows Bluepad32 settings, button mappings, and known devices to persist on LittleFS. C++ conversion encapsulates motor/steering state. LVGL buffer reduction was critical to fix BLE controller init failure caused by heap exhaustion (only 15.7 KB free of 73.9 KB with original 20-line buffers, BLE controller needs ~30 KB).

### What it does / expected behaviour
- DFS active on boot: CPU scales between 32–96 MHz on ESP32-H2 (logged as "DFS 32-96 MHz")
- BLE console accepts commands via NUS from any BLE terminal app (e.g., nRF Connect)
- Console output mirrored to both UART and BLE when NUS client is connected
- INI config files created with defaults on first boot under `/littlefs/config/`
- `cfgload` and `cfgsave` commands manage Bluepad32 config at runtime
- Motor and steering objects use C++ internally with unchanged C API
- LVGL renders with 10-line buffers (2 × 9,600 bytes), freeing ~19 KB for BLE stack
- BLE controller initialises successfully: BTstack up and running, Bluepad32 scanning

### Verified
- Build: OK — ESP32-H2, `idf.py build` clean
- Flash: OK — COM13, hash verified
- Debug: OK — Serial monitor confirms: "DFS 32-96 MHz", BLE MAC 10:51:DB:60:AF:04, "BTstack up and running", "Bluepad32 init complete — scanning for gamepads", console responsive, blinky cycling

## [2026-03-28] UART Console, Motor Objects, Ackermann Steering, XMODEM

### What was changed
- `main/motor.h` — Created motor struct and API for 4-wheel drive (FL, FR, RL, RR) with bobbycar parameters (THROTTLE_MAX=1000, HOVER_START_FRAME=0x7A7A)
- `main/motor.c` — Created 4 motor instances with clamped torque set/get, per-motor enable, set-all-torque batch function
- `main/steering_algo.h` — Created Ackermann steering geometry API with bobbycar dimensions (wheelbase=35 cm, width=30 cm)
- `main/steering_algo.c` — Ported `calc_torque_per_wheel()` from bobbycar-project; Ackermann torque distribution with PID correction and per-wheel clamping
- `main/console.h` — Created shell_cmd_t typedef (1180-style: name+desc+handler), console init/register/exec API
- `main/console.c` — Created UART console task (FreeRTOS fgets loop, bobbycar exec dispatch pattern), 1180-format help command, command registration framework
- `main/console_cmds.h` — Created registration function prototypes for motor/steering and filesystem command groups
- `main/console_cmds.c` — Created 20 bobbycar-style commands: echo, ver, reset, sets/gets/getds (steering), sett/gett (throttle), getb (board), seti/geti (input source), setm/getm (mode), setkp/ki/kd/getkp/ki/kd/getpo (PID)
- `main/xmodem.h` — Created XMODEM-CRC protocol API (SOH/STX, CRC-16 poly 0x1021, error codes)
- `main/xmodem.c` — Created full XMODEM-CRC send/receive with 3s timeout, 10 retries, 128-byte and 1K block support
- `main/console_cmds_fs.c` — Created 9 filesystem commands (ls, cd, pwd, rm, mkdir, show, recv, send, format) operating on LittleFS with XMODEM file transfer and path-traversal protection
- `main/main.c` — Added motor_init(), console_cmds_register(), console_cmds_fs_register(), console_init() calls; version bumped to v0.5
- `main/CMakeLists.txt` — Added 6 new source files: motor.c, steering_algo.c, console.c, console_cmds.c, console_cmds_fs.c, xmodem.c
- `Documentation/Requirements/requirements.md` — Added requirements #11-14 (UART Console Shell, XMODEM File Transfer, Four-Wheel Motor Objects, Ackermann Steering Algorithm)
- `Documentation/ToDo/console-motors-steering.md` — Created feature todo checklist

### Why it was changed
Feature addition: ports the console shell, motor control, and Ackermann steering algorithm from the bobbycar-project. Console uses the 1180-project help structure (name+description columns) with the bobbycar dispatch pattern. XMODEM-CRC enables file transfer over UART for firmware data and configuration files.

### What it does / expected behaviour
- UART console task starts on boot, prints `> ` prompt, accepts typed commands
- `help` lists all 29 commands with 1180-style aligned descriptions
- Motor commands (sets/sett) automatically run Ackermann torque distribution across 4 wheels
- Steering angle + throttle → per-wheel torque via calc_torque_per_wheel()
- XMODEM recv/send transfers files over serial to/from LittleFS
- Filesystem commands (ls, cd, pwd, rm, mkdir, show, format) with ".." traversal protection
- 4 motor objects initialised: FL, FR, RL, RR with torque clamping ±1000

### Verified
- Build: OK — ESP32-H2, binary 0x11f9b0 bytes (63% free in 3 MB partition)
- Flash: OK — COM13, hash verified
- Debug: OK — Serial monitor confirms: "Motor init: 4 motors (FL FR RL RR)", "Console ready (type 'help')", "Console task started", prompt displayed

## [2026-03-28] LittleFS Storage, Console Removal, Bluepad32 Fork Switch

### What was changed
- `.gitmodules` — Changed Bluepad32 submodule URL from `ricardoquesada/bluepad32` to `Vicy-DE/bluepad32` fork
- `.gitignore` — Added `build*/` pattern to ignore all numbered build directories (build2/, build3/, etc.)
- `partitions.csv` — Created custom partition table: nvs (24 KB) + phy_init (4 KB) + factory (3 MB) + littlefs (960 KB) = 4 MB
- `main/storage.h` — Created LittleFS storage API: gamepad database (save/load/remove) and key=value config persistence
- `main/storage.c` — Created LittleFS implementation: VFS mount with auto-format, binary gamepad database with FIFO eviction, config files with path-traversal validation
- `main/my_platform.c` — Added LittleFS storage integration: loads known gamepads on init (logs count + addresses), saves gamepads on device ready; removed `uni_bt_del_keys_unsafe()` for persistent pairing
- `main/main.c` — Added `storage_init()` call before Bluepad32; removed `btstack_stdio_init()` and `#include <btstack_stdio_esp32.h>` (console removal); version bumped to v0.4
- `main/CMakeLists.txt` — Added `storage.c` to SRCS, `joltwallet__littlefs` to PRIV_REQUIRES
- `main/idf_component.yml` — Added `joltwallet/littlefs: "^1.20.4"` managed component dependency
- `sdkconfig.defaults` — Switched from `CONFIG_PARTITION_TABLE_SINGLE_APP_LARGE` to `CONFIG_PARTITION_TABLE_CUSTOM` with `partitions.csv`; removed "console commands" comment from FreeRTOS stats

### Why it was changed
Feature addition: persistent storage for gamepad configs and known devices via LittleFS filesystem. Submodule switched to user's own fork for future customization. Bluepad32 console completely removed (was already disabled via Kconfig but stdio init still ran). Build directory gitignore expanded to cover numbered build dirs from multi-target workflows.

### What it does / expected behaviour
- LittleFS partition (960 KB) auto-formats on first boot, mounts at `/littlefs`
- Known gamepads persist across reboots: address and name saved when a gamepad becomes ready
- Gamepad database holds up to 8 entries with FIFO eviction when full
- Config key=value files stored under `/littlefs/config/` with input validation
- BT pairing keys persist (no longer deleted on every boot)
- No console/stdio init — saves RAM, eliminates UART conflict risk
- Binary size: 1.1 MB (63% free in 3 MB partition)

### Verified
- Build: OK — ESP32-H2, 1746/1746 targets, binary 0x119410 bytes
- Flash: OK — COM13, hash verified
- Debug: OK — Serial monitor confirms: "LittleFS mounted: total=983040 used=16384", "Loaded 0 known gamepad(s) from storage", "Bluepad32 init complete — scanning for gamepads", blinky cycling, no console crash

## [2026-03-28] Bluetooth Gamepad Support — Bluepad32 (BLE)

### What was changed
- `components/bluepad32/` — Added Bluepad32 v4.2.0 as git submodule; integrated BTstack via `integrate_btstack.py`
- `CMakeLists.txt` — Added Bluepad32 + BTstack component directories (`EXTRA_COMPONENT_DIRS`), `-Wno-format` for third-party code, and `CONFIG_BT_LE_MSYS_BUF_FROM_HEAP=1` compile definition for `__idf_bt` on ESP32-H2
- `main/main.c` — Added BTstack/Bluepad32 includes, moved sensor/display loop to `sensor_display_task()` FreeRTOS task, `app_main` now initializes peripherals then starts BTstack (`btstack_run_loop_execute()` blocks), version bumped to v0.3
- `main/my_platform.c` — Created custom Bluepad32 platform "bobbycar" with callbacks: init, device discovered (filters keyboards), connected/disconnected, ready (assigns GAMEPAD_SEAT_A, triggers rumble), controller data (logs axes/buttons)
- `main/CMakeLists.txt` — Added `bluepad32` and `btstack` to `PRIV_REQUIRES`, added `my_platform.c` to `SRCS`
- `sdkconfig.defaults` — Added BLE controller settings (`CONFIG_BT_ENABLED`, `CONFIG_BT_CONTROLLER_ONLY`), Bluepad32 settings (`CONFIG_BLUEPAD32_PLATFORM_CUSTOM`, `CONFIG_BLUEPAD32_MAX_DEVICES=4`, `CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE=n`), BTstack (`CONFIG_BTSTACK_AUDIO=n`), partition table (`CONFIG_PARTITION_TABLE_SINGLE_APP_LARGE`), FreeRTOS stats
- `sdkconfig` — Updated to match sdkconfig.defaults (Bluepad32 console disabled, BT enabled, partition table enlarged)
- `Documentation/Requirements/requirements.md` — Added Req #10 (Bluetooth Gamepad Support)
- `Documentation/ToDo/bluetooth-gamepad.md` — Created feature todo checklist

### Why it was changed
Feature addition: Bluetooth gamepad support via Bluepad32 library with BTstack BLE stack. Enables Xbox Wireless controller (BLE models) input for the Bobbycar steering controller. ESP32-H2 supports BLE 5 only (no Bluetooth Classic), so only BLE-capable gamepads are supported.

### What it does / expected behaviour
- BTstack initializes BLE controller, registers VHCI transport (synchronous mode on ESP32-H2)
- Bluepad32 starts scanning for BLE gamepads on boot
- Custom platform "bobbycar" filters out keyboards, accepts gamepads
- On gamepad connect: assigns GAMEPAD_SEAT_A, triggers 150 ms rumble feedback
- Controller data callback logs axes (X/Y/RX/RY) and button bitmask
- Sensor/display loop continues independently in its own FreeRTOS task
- WS2812 blinky task continues cycling R→G→B alongside BLE scanning

### Bugs fixed during integration
- **UART console crash:** Bluepad32's `uni_console_init()` clashed with existing UART — fixed with `CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE=n`
- **msys_init failed -4:** ESP32-H2 Kconfig lacks `CONFIG_BT_LE_MSYS_BUF_FROM_HEAP` — injected via CMake `target_compile_definitions`
- **BTstack HCI stuck:** ESP32-H2 requires synchronous VHCI mode (not async like ESP32/C3/S3) — confirmed original btstack_port_esp32.c code was correct

### Verified
- Build: OK — ESP32-H2, binary ~1.13 MB (27% free in 1.5 MB partition)
- Flash: OK — COM13, hash verified
- Debug: OK — Serial monitor confirmed: "Bluepad32 init complete — scanning for gamepads", BLE MAC 10:51:db:60:af:04, blinky continues cycling. Known non-fatal warnings: "Controller lib version mismatch!" (ESP-IDF v5.4.3 vs BTstack), "Failed command: opcode 0x0c05" (HCI_Set_Event_Filter is BR/EDR-only, expected to fail on BLE-only H2).

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
