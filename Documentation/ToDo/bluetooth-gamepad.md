# Todo: Bluetooth Gamepad Support

**Created:** 2026-03-28
**Requirement:** [Req #10 — Bluetooth Gamepad Support](../Requirements/requirements.md)
**Status:** Verified on hardware

---

## Tasks

- [x] Research Bluepad32 library compatibility with ESP32-H2 (BLE only)
- [x] Confirm Xbox Wireless controller BLE support (model 1914 native BLE, model 1708 with fw v5+)
- [x] Update requirements.md with Req #10
- [x] Clone Bluepad32 as git submodule under `components/bluepad32/`
- [x] Integrate BTstack via `integrate_btstack.py`
- [x] Update root CMakeLists.txt to include Bluepad32 + BTstack component directories
- [x] Update main/CMakeLists.txt to add bluepad32 and btstack dependencies
- [x] Create custom Bluepad32 platform implementation (`my_platform.c`)
  - [x] Implement `on_init_complete` — start scanning and autoconnect
  - [x] Implement `on_device_connected` / `on_device_disconnected` — log events
  - [x] Implement `on_device_ready` — assign gamepad seat
  - [x] Implement `on_controller_data` — log gamepad axes and buttons
- [x] Restructure `app_main` to support BTstack main loop
  - [x] Move sensor/display loop to a FreeRTOS task
  - [x] Initialize BTstack and Bluepad32 in `app_main`
  - [x] Call `btstack_run_loop_execute()` as last statement (blocks)
- [x] Fix UART console crash (`CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE=n`)
- [x] Fix msys_init -4 (`CONFIG_BT_LE_MSYS_BUF_FROM_HEAP=1` via CMake)
- [x] Fix BTstack HCI stuck (ESP32-H2 uses synchronous VHCI, not async)
- [x] Build passes with feature integrated (`idf.py build`)
- [x] Flashed and verified on hardware (`idf.py flash monitor`)
- [x] CHANGE_LOG.md updated
- [x] PROJECT_DOC.md updated
- [ ] Test with physical Xbox Wireless controller (BLE model)
- [ ] Map gamepad axes/buttons to steering control output

---

## Notes

- ESP32-H2 has NO Bluetooth Classic (BR/EDR), only BLE 5 + 802.15.4
- Xbox Wireless controller model 1914 (3-button): BLE native, firmware v5.15+
- Xbox Wireless controller model 1708 (2-button): BLE since firmware v5.x (BR/EDR before v5)
- Xbox Adaptive Controller: BLE native
- Bluepad32 uses BTstack which is NOT multithreaded — all BP32/BTstack calls must happen from BTstack thread
- BTstack commercial license required for closed-source products (Apache 2 for open source)
- `btstack_run_loop_execute()` blocks forever — sensor/display must run in separate task
