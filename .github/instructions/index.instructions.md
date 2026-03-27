---
applyTo: "**/*.c,**/*.h,**/*.S,**/CMakeLists.txt,**/*.bat,**/*.ps1,**/*.py,**/*.sh,**/sdkconfig*,**/Kconfig*"
---

# Bobbycar-Steering — Copilot Instructions

## After Every Code Change — MANDATORY

1. **Build** → `idf.py -DIDF_TARGET=esp32c3 build` — fix errors before continuing. [BUILD](BUILD/BUILD_README.instructions.md)
2. **Flash** → `idf.py -p COMx flash` or JTAG via OpenOCD. [DEBUG](DEBUG/DEBUG_README.instructions.md)
3. **Verify** → serial monitor: `idf.py -p COMx monitor` · check expected log output. [DEBUG §6](DEBUG/DEBUG_README.instructions.md)
4. **Document** → append `Documentation/CHANGE_LOG.md` + update `Documentation/PROJECT_DOC.md`. [CHANGE_DOC](documentation/CHANGE_DOC.instructions.md) · [PROJECT_DOC](documentation/PROJECT_DOC.instructions.md)
5. **Test** → generate tests, run on hardware, save report in `Documentation/Tests/`. [TEST_DOC](documentation/TEST_DOC.instructions.md)
6. **Commit** → stage relevant files, write Conventional Commit message — **never push**. [COMMIT](documentation/COMMIT.instructions.md)

## Before New Features

1. Update `Documentation/Requirements/requirements.md` → [REQUIREMENTS_DOC](documentation/REQUIREMENTS_DOC.instructions.md)
2. Create `Documentation/ToDo/<feature>.md` → [TODO_DOC](documentation/TODO_DOC.instructions.md)

## On Hardware / Pin Changes

1. Read relevant datasheets in `datasheets/` before touching GPIO or peripheral config.
2. Update `Documentation/PINOUT_<MCU>.md` for every affected platform.
3. Mirror pin changes across all target platforms (ESP32-H2, ESP32-C3, ESP32-C5).
4. [HARDWARE](HARDWARE/HARDWARE.instructions.md)

## Rules

- Scripts: test/debug in `Target/`, build tools in `tools/`. [SCRIPTS](CODING/SCRIPTS.instructions.md)
- Coding: [COMMENTS](CODING/COMMENTS.instructions.md)
- Hardware: [HARDWARE](HARDWARE/HARDWARE.instructions.md) — consult `datasheets/`, update `PINOUT_*.md`.
- Cross-platform: code in `main/` must compile for all three targets (ESP32-H2, ESP32-C3, ESP32-C5). Use `#if CONFIG_IDF_TARGET_*` for target-specific code.
- LVGL: UI code uses LVGL library from `components/lvgl/`. Display driver in `components/`.
- ESP-IDF: use `idf.py` commands. Set `IDF_PATH` to `sdk/esp-idf`. Set target via `idf.py set-target`.
- Flash via USB-CDC built into ESP32-C3/C5/H2 SuperMini boards — no external programmer needed.
