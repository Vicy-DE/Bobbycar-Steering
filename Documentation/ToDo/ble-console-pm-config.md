# Todo: BLE Console, Power Management, INI Config, C++ Conversion

**Created:** 2026-03-29
**Requirement:** [Req #15-19 — C++, PM, INI Config, BLE Console](../Requirements/requirements.md)
**Status:** Done

---

## Tasks

- [x] C++ conversion of motor.c → motor.cpp
  - [x] Motor class with private members and const getters
  - [x] MotorController singleton managing 4 instances
  - [x] extern "C" wrappers for C interop
- [x] C++ conversion of steering_algo.c → steering_algo.cpp
  - [x] Uses `<cmath>`, `static_cast<>`
  - [x] C-linkage API preserved
- [x] Power management (DFS)
  - [x] power_mgmt.h/c created
  - [x] Per-target min/max frequencies (H2: 32-96, C3: 80-160, C5: 80-240)
  - [x] Light sleep disabled (BTstack VHCI incompatible)
  - [x] sdkconfig.defaults: PM_ENABLE, TICKLESS_IDLE, BT_LE_SLEEP_ENABLE
- [x] Bluepad32 INI configuration
  - [x] ini_parser.h/c — minimal INI parser
  - [x] bp32_config.h/c — general, buttonmap, devices config
  - [x] Default config creation on first boot
  - [x] cfgload/cfgsave console commands
- [x] BLE console (Nordic UART Service)
  - [x] ble_console.h/c — BTstack GATT NUS implementation
  - [x] TX ring buffer, RX line buffer
  - [x] console_printf() dual UART+BLE output
  - [x] NUS service added to Bluepad32 GATT database
  - [x] Integrated with existing console_exec() dispatch
- [x] LVGL buffer fix — reduced LVGL_BUF_LINES from 20 to 10
  - [x] Root cause: 38.4 KB consumed by display buffers left only 15.7 KB for BLE
  - [x] Fix saves 19.2 KB, BLE controller init succeeds with 34.9 KB free
- [x] Build passes with feature integrated (`idf.py build`)
- [x] Flashed and verified on hardware (`idf.py flash monitor`)
- [x] CHANGE_LOG.md updated
- [x] requirements.md updated (if scope changed)

---

## Notes

- Light sleep causes BTstack VHCI HCI transport to fail during init. DFS + WFI provides adequate power savings.
- ESP32-H2 heap is tight (73.9 KB total, ~35 KB free after display+LittleFS). Monitor heap usage when adding features.
- BLE modem sleep is still active via CONFIG_BT_LE_SLEEP_ENABLE=y (handled by BLE controller independently).
