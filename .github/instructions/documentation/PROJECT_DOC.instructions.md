---
applyTo: "**/*.c,**/*.h,**/*.S,**/CMakeLists.txt"
---

# Project Documentation — Instructions for Copilot

## When to execute

**Step 4 of the workflow — after a successful debug session, run after CHANGE_DOC.**
Update the project documentation whenever the overall architecture, module responsibilities, key interfaces, or system behaviour change.

## Target files

Meaningful file names in `Documentation`.
Overwrite / update the relevant sections in-place. Create the file if it does not exist.

---

## Document structure (keep this structure)

```markdown
# Project Documentation — Bobbycar-Steering

**Last updated:** YYYY-MM-DD
**IDF Target:** `esp32c3` / `esp32h2` / `esp32c5`
**ESP-IDF Version:** v5.x

---

## 1. Project Overview
<High-level purpose of the firmware. What product/system does it implement?>

## 2. Hardware Platform
<MCU variants, SuperMini boards, peripherals in use, connectivity.>

## 3. Software Architecture
<ESP-IDF components, FreeRTOS tasks, LVGL integration, steering control.>

## 4. Key Modules

| Module / Component | Responsibility |
|---|---|
| `main/main.c` | Application entry point — task creation, system init |
| `components/steering/` | Steering control logic — PWM/servo output |
| `components/display/` | Display driver — SPI/I2C to LCD/OLED via LVGL |
| `components/lvgl/` | LVGL graphics library (submodule) |
| ... | ... |

## 5. Build System
<ESP-IDF CMake, idf.py commands, target switching, sdkconfig.>

## 6. Flashing & Debug Toolchain
<USB-CDC, OpenOCD, idf.py monitor, JTAG.>

## 7. Cross-Platform Support
<ESP32-C3, ESP32-H2, ESP32-C5 differences and handling.>

## 8. Known Limitations / Open Issues
<Current known bugs, platform-specific quirks, workarounds, TODOs.>

## 9. Revision History (summary)
| Date | Summary |
|---|---|
| YYYY-MM-DD | Initial documentation |
```

---

## Rules

- **MUST** update the "Last updated" date on every edit.
- **MUST** update section 4 (Key Modules) whenever files are added, removed, or responsibilities change.
- **MUST** update section 8 (Known Limitations) to reflect resolved issues and newly discovered ones.
- **MUST** add a one-line entry to section 9 (Revision History) for each update session.
- **SHOULD** keep descriptions concise — prefer tables and bullet lists over long paragraphs.
- **MUST NOT** duplicate information already in CHANGE_LOG.md — link to it instead.
- **MUST NOT** describe implementation details that belong in code comments.
