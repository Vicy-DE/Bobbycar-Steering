# Project Documentation — Bobbycar-Steering

**Last updated:** 2026-03-29 (v0.7)
**IDF Target:** `esp32c3` / `esp32h2` / `esp32c5`
**ESP-IDF Version:** v5.4.3

---

## 1. Project Overview

Cross-platform steering controller firmware for the Bobbycar project. Targets ESP32 SuperMini development boards (ESP32-C3, ESP32-H2, ESP32-C5) with LVGL-based display output and PWM servo control. Built on ESP-IDF v5.4 with FreeRTOS.

Current state: ADC sensor readout, ST7796S 3.5" SPI TFT display with LVGL dashboard UI, WS2812 RGB LED blinky, BLE gamepad input via Bluepad32 with INI-based configuration, BLE console (Nordic UART Service), LittleFS persistent config/gamepad storage, UART console shell with 29+ commands, XMODEM-CRC file transfer, 4-wheel motor objects (C++), Ackermann steering algorithm (C++), and dynamic frequency scaling power management.

**Hardware source:** [AliExpress — ESP32 SuperMini Dev Boards](https://de.aliexpress.com/item/1005009495310442.html)

---

## 2. Hardware Platform

### Target SoCs

| SoC | CPU | Clock | SRAM | Flash | Connectivity |
|-----|-----|-------|------|-------|-------------|
| ESP32-C3 | 32-bit RISC-V single-core | 160 MHz | 400 KB | 4 MB SiP | Wi-Fi 4 (802.11 b/g/n) + BLE 5 |
| ESP32-H2 | 32-bit RISC-V single-core | 96 MHz | 320 KB | 2/4 MB SiP | BLE 5 + IEEE 802.15.4 (Zigbee/Thread) |
| ESP32-C5 | 32-bit RISC-V single-core | 240 MHz | 384 KB | External PSRAM support | Wi-Fi 6 (802.11ax, 2.4+5 GHz) + BLE 5 + 802.15.4 |

### Development Board

ESP32 SuperMini form factor with:
- USB Type-C connector (USB-CDC for programming and serial)
- Built-in USB-JTAG on C3/C5 for debugging
- Minimal external components (LED, BOOT/RST buttons)
- Castellated pads for breadboard or SMD mounting

### Debug Interface

- **Flashing:** USB-CDC via built-in USB (no external programmer needed)
- **JTAG:** Built-in USB-JTAG on ESP32-C3 and ESP32-C5
- **Serial:** USB-CDC at 115200 baud for log output

---

## 3. Software Architecture

```
┌──────────────────────────────────────────┐
│            ESP-IDF v5.4.3                │
│  ┌──────────┐  ┌──────────┐  ┌────────┐ │
│  │ FreeRTOS │  │ BLE 5    │  │ VHCI   │ │
│  └──────────┘  └──────────┘  └────────┘ │
├──────────────────────────────────────────┤
│              Components                  │
│  ┌──────────┐  ┌──────────┐  ┌────────┐ │
│  │ steering │  │ display  │  │bluepad │ │
│  │  (PWM)   │  │(LVGL+drv)│  │+btstack│ │
│  └──────────┘  └──────────┘  └────────┘ │
├──────────────────────────────────────────┤
│                main/                     │
│         app_main() entry                 │
│  ┌─────────────┐ ┌──────────┐ ┌────────┐ │
│  │sensor_displ │ │ BTstack  │ │console │ │
│  │_task (RTOS) │ │  (block) │ │  task  │ │
│  └─────────────┘ └──────────┘ └────────┘ │
│  ┌─────────────┐ ┌──────────┐ ┌────────┐ │
│  │  motor (4x) │ │ steering │ │ xmodem │ │
│  │  FL/FR/RL/RR│ │ Ackerman │ │  CRC   │ │
│  └─────────────┘ └──────────┘ └────────┘ │
└──────────────────────────────────────────┘
```

### Boot Flow

1. ESP-IDF bootloader loads application
2. `app_main()` initialises peripherals (ADC, display, WS2812)
3. Motor objects initialised (4 wheels: FL, FR, RL, RR)
4. LittleFS mounted, Bluepad32 INI config loaded
5. Power management enabled (DFS, CPU scales to min frequency when idle)
6. Console commands registered, UART console task started
7. FreeRTOS tasks created: `sensor_display_task`, `blinky_task`
8. BTstack + Bluepad32 initialised; BLE console (NUS) registered
9. `btstack_run_loop_execute()` blocks `app_main` forever
10. BLE scanning starts, gamepad events and NUS console handled via callbacks

---

## 4. Key Modules

| Module / Component | Responsibility |
|---|---|
| `main/main.c` | Application entry point — peripheral init, FreeRTOS task creation, BTstack/Bluepad32 init, blocks on `btstack_run_loop_execute()` |
| `main/my_platform.c` | Custom Bluepad32 platform "bobbycar" — gamepad discovery, connect/disconnect, controller data callbacks, LittleFS persistence, BLE console + INI config init |
| `main/storage.h` / `main/storage.c` | LittleFS-based persistent storage — gamepad database (up to 8 entries), key=value config files |
| `main/pin_config.h` | Per-target GPIO pin assignments (SPI display, I2C touch, TWAI/CAN, ADC, WS2812, blinky GPIOs) via `#if CONFIG_IDF_TARGET_*` guards |
| `components/display/` | ST7796S 3.5" SPI TFT driver — SPI bus init, esp_lcd panel, LVGL v9 flush integration, backlight, landscape rotation |
| `components/bluepad32/` | Bluepad32 v4.2.0 (git submodule) — BLE gamepad library with BTstack integration |
| `main/motor.h` / `main/motor.cpp` | 4-wheel motor objects (FL, FR, RL, RR) — C++ Motor class with torque set/get, enable, batch set, clamped to ±1000; extern "C" wrappers for C callers |
| `main/steering_algo.h` / `main/steering_algo.cpp` | Ackermann steering algorithm — C++ `calc_torque_per_wheel()` distributes throttle + steering angle to per-wheel torque |
| `main/console.h` / `main/console.c` | UART console shell — FreeRTOS task, command dispatch (bobbycar pattern), 1180-style help, registration framework |
| `main/console_cmds.h` / `main/console_cmds.c` | 20+ motor/steering/PID/system commands — sets, gets, sett, gett, setkp/ki/kd, cfgload, cfgsave, etc. |
| `main/console_cmds_fs.c` | 9 filesystem commands — ls, cd, pwd, rm, mkdir, show, recv, send, format (LittleFS + XMODEM) |
| `main/xmodem.h` / `main/xmodem.c` | XMODEM-CRC protocol — 128B/1K blocks, CRC-16 (poly 0x1021), 3s timeout, 10 retries |
| `main/power_mgmt.h` / `main/power_mgmt.c` | ESP32 power management — DFS (dynamic frequency scaling), CPU scales to min freq when idle + WFI |
| `main/ble_console.h` / `main/ble_console.c` | BLE console via Nordic UART Service (NUS) — BTstack GATT server, TX ring buffer, mirrors console output to UART + BLE |
| `main/bp32_config.h` / `main/bp32_config.c` | Bluepad32 INI configuration — loads/saves general, button mapping, and known device configs from LittleFS |
| `main/ini_parser.h` / `main/ini_parser.c` | Minimal INI file parser — `[sections]`, `key=value`, comments; read and write operations |
| `components/steering/` | Steering servo control — PWM output via LEDC peripheral (planned) |
| `components/lvgl/` | LVGL v9.2 graphics library (git submodule) |

---

## 5. Build System

ESP-IDF CMake-based build system with `idf.py` frontend.

| Command | Purpose |
|---------|---------|
| `idf.py set-target esp32c3` | Select target SoC |
| `idf.py build` | Build firmware |
| `idf.py -p COMx flash` | Flash via USB-CDC |
| `idf.py -p COMx monitor` | Serial monitor (115200 baud) |
| `idf.py menuconfig` | Configure sdkconfig options |
| `idf.py fullclean` | Clean build directory |

### Configuration

- `sdkconfig.defaults` — common defaults for all targets
- `sdkconfig.defaults.esp32c3` — C3-specific defaults (if needed)
- `sdkconfig.defaults.esp32h2` — H2-specific defaults (if needed)
- `sdkconfig.defaults.esp32c5` — C5-specific defaults (if needed)
- `partitions.csv` — custom partition table: nvs + phy_init + factory (3 MB) + littlefs (960 KB)

---

## 6. Flashing & Debug Toolchain

| Tool | Source |
|------|--------|
| idf.py | ESP-IDF build/flash/monitor frontend |
| OpenOCD | Bundled with ESP-IDF for JTAG debug |
| GDB | `riscv32-esp-elf-gdb` installed by ESP-IDF |

### Flash via USB-CDC

```powershell
idf.py -p COM<N> flash monitor
```

If auto-reset fails: hold BOOT → press RST → release BOOT → flash.

---

## 7. Cross-Platform Support

| Feature | ESP32-C3 | ESP32-H2 | ESP32-C5 |
|---------|----------|----------|----------|
| Wi-Fi | 802.11 b/g/n | — | 802.11ax (Wi-Fi 6) |
| Bluetooth | BLE 5 | BLE 5 | BLE 5 |
| 802.15.4 | — | Zigbee/Thread | Zigbee/Thread |
| Max Clock | 160 MHz | 96 MHz | 240 MHz |
| GPIOs | 22 | 19 | 29 |
| USB | USB-Serial/JTAG | USB-Serial/JTAG | USB-Serial/JTAG |

Target-specific code uses `#if CONFIG_IDF_TARGET_ESP32C3` / `ESP32H2` / `ESP32C5` guards.

---

## 8. Known Limitations / Open Issues

- Steering servo PWM output not yet implemented (Ackermann algorithm is ready, motor objects exist but no hardware driver yet)
- Touch driver not yet implemented (I2C pins reserved)
- ESP32-C5 support in ESP-IDF may be in preview status
- ESP32-H2 has no Wi-Fi — connectivity limited to BLE and 802.15.4
- Display colours may require MADCTL tuning per physical display revision
- ADC readings are raw 12-bit values — no calibration or voltage conversion applied yet
- Bluepad32 "Controller lib version mismatch!" warning on ESP32-H2 (non-fatal, ESP-IDF v5.4.3 controller vs BTstack expectation)
- HCI_Set_Event_Filter (opcode 0x0c05) fails on ESP32-H2 — expected, BR/EDR-only command on BLE-only SoC
- ESP32-H2 requires `CONFIG_BT_LE_MSYS_BUF_FROM_HEAP=1` injected via CMake (Kconfig does not expose it)
- BTstack `integrate_btstack.py` output is gitignored — must re-run after fresh clone (see setup.ps1)
- Gamepad support not yet tested with physical Xbox controller
- **LVGL buffer constraint:** `LVGL_BUF_LINES` must stay ≤ 10 on ESP32-H2 (320 KB SRAM). Two DMA buffers at 20 lines consumed 38.4 KB, leaving insufficient heap for BLE controller init (~30 KB needed). Reduced to 10 lines (2 × 9.6 KB = 19.2 KB total) to free memory.
- **Light sleep incompatible with BTstack VHCI:** Enabling `light_sleep_enable = true` causes BTstack HCI transport to fail during initialization ("Failed to initialize HCI"). DFS is used instead (CPU scales to 32 MHz when idle + WFI).
- CAN bus communication not yet implemented (TWAI pins reserved, architecture documented)

---

## 9. CAN Bus Architecture — Bobbycar Network

The Bobbycar uses a CAN bus network connecting the ESP32 steering controller to multiple peripherals. This section documents the planned architecture for future implementation.

### 9.1 Network Topology

```
                      CAN Bus (TWAI)
    ┌──────────┬──────────┬──────────┬──────────┬──────────┐
    │          │          │          │          │          │
┌───┴───┐ ┌───┴───┐ ┌───┴───┐ ┌───┴───┐ ┌───┴───┐ ┌───┴───┐
│ESP32  │ │VESC   │ │VESC   │ │VESC   │ │VESC   │ │Trailer│
│(Main) │ │  FL   │ │  FR   │ │  RL   │ │  RR   │ │(opt.) │
└───────┘ └───────┘ └───────┘ └───────┘ └───────┘ └───┬───┘
    │                                                   │
    │ CAN                                          CAN/I2C
    │                                                   │
┌───┴───────┐                                     ┌─────┴─────┐
│ CH32V203  │                                     │  Trailer  │
│ Steering  │                                     │   MCU     │
│ Sensor MCU│                                     └───────────┘
└───┬───────┘
    │ I2C
    │
┌───┴───────┐
│ CH32V003  │
│  Lights   │
│ Controller│
└───────────┘
```

### 9.2 Node Descriptions

| Node | MCU | Role | Interface |
|------|-----|------|-----------|
| **Main Controller** | ESP32-H2/C3/C5 | Steering controller, display, BLE gamepad, console | CAN (TWAI) master |
| **VESC FL** | — | Front-left motor controller (BLDC ESC) | CAN slave |
| **VESC FR** | — | Front-right motor controller (BLDC ESC) | CAN slave |
| **VESC RL** | — | Rear-left motor controller (BLDC ESC) | CAN slave |
| **VESC RR** | — | Rear-right motor controller (BLDC ESC) | CAN slave |
| **Steering Sensor** | CH32V203 | ADC steering angle sensor + other sensors | CAN slave + I2C master |
| **Lights Controller** | CH32V003 | LED light control | I2C slave (via CH32V203 bridge) |
| **Trailer** | TBD | Optional trailer unit | CAN slave |

### 9.3 TWAI Pin Assignments

| Target | TWAI_TX | TWAI_RX |
|--------|---------|---------|
| ESP32-C3 | GPIO2 | GPIO3 |
| ESP32-H2 | GPIO0 | GPIO3 |
| ESP32-C5 | GPIO2 | GPIO3 |

### 9.4 Communication Protocols

**ESP32 ↔ VESC (CAN):** Standard VESC CAN protocol for torque/speed commands and telemetry (voltage, current, RPM, temperature). Each VESC has a unique CAN ID (1–4).

**ESP32 ↔ CH32V203 (CAN):** Custom CAN messages for steering angle sensor data, auxiliary sensor readings, and firmware update (IAP) commands.

**CH32V203 ↔ CH32V003 (I2C):** The CH32V203 acts as a CAN→I2C bridge. The ESP32 sends light control commands or CH32V003 firmware data over CAN to the CH32V203, which relays them to the CH32V003 via I2C.

**ESP32 ↔ Trailer (CAN):** Optional CAN node for trailer unit. Supports control commands and firmware update via CAN IAP.

### 9.5 Firmware Update via CAN (IAP)

The ESP32 can update firmware on the following devices using custom In-Application Programming (IAP) protocols over the CAN bus:

| Target Device | Update Path | Protocol |
|---------------|------------|----------|
| CH32V203 (steering sensor) | ESP32 → CAN → CH32V203 | Custom CAN IAP |
| CH32V003 (lights) | ESP32 → CAN → CH32V203 → I2C → CH32V003 | CAN→I2C bridge IAP |
| Trailer MCU | ESP32 → CAN → Trailer | Custom CAN IAP |

Firmware binary files can be transferred to the ESP32 via BLE (NUS + XMODEM from Android app) or UART (XMODEM), then relayed to target devices over CAN.

---

## 10. Revision History (summary)

| Date | Summary |
|---|---|
| 2026-03-29 | Power management (DFS), BLE console (NUS), INI config, C++ conversion, LVGL buffer fix, CAN bus architecture docs |
| 2026-03-28 | UART console shell (29 cmds), XMODEM-CRC, 4-wheel motor objects, Ackermann steering |
| 2026-03-28 | LittleFS storage, console removal, Bluepad32 fork switch |
| 2026-03-28 | Bluetooth gamepad support via Bluepad32 + BTstack (BLE scanning verified on ESP32-H2) |
| 2026-03-28 | WS2812 RGB LED blinky fix (RMT driver) + GPIO blinky task |
| 2026-03-27 | ADC sensor readout + ST7796S display driver + LVGL dashboard UI + pinout docs |
| 2026-03-27 | Initial project setup — ESP-IDF + LVGL submodules, cross-platform structure, requirements |
