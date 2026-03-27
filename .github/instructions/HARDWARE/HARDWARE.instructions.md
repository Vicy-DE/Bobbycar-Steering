---
applyTo: "**/*.c,**/*.h,**/*.S,**/CMakeLists.txt"
---

# Hardware Change Rules

## On ANY Pin or Peripheral Change — MANDATORY

1. **Read datasheets** — Open and consult every relevant datasheet/reference manual from `datasheets/` before making GPIO, clock, or peripheral changes. Never guess pin alternate-function mappings.
2. **Update PINOUT.md** — Every platform that is affected must have its `Documentation/PINOUT_<MCU>.md` updated to reflect the new pin assignment, mode, and function.
3. **Cross-check all targets** — ESP32-H2, ESP32-C3, and ESP32-C5 have different GPIO counts and alternate functions. A pin change must be verified against all three target datasheets.

## Datasheet Sources

All datasheets and reference manuals live in `datasheets/`. The following documents MUST be consulted:

| MCU | Document | File |
|-----|----------|------|
| ESP32-C3 | Datasheet | `datasheets/esp32-c3_datasheet_en.pdf` |
| ESP32-C3 | Technical Reference Manual | `datasheets/esp32-c3_technical_reference_manual_en.pdf` |
| ESP32-H2 | Datasheet | `datasheets/esp32-h2_datasheet_en.pdf` |
| ESP32-H2 | Technical Reference Manual | `datasheets/esp32-h2_technical_reference_manual_en.pdf` |
| ESP32-C5 | Datasheet | `datasheets/esp32-c5_datasheet_en.pdf` |
| ESP32-C5 | Technical Reference Manual | `datasheets/esp32-c5_technical_reference_manual_en.pdf` |

> **If a datasheet is missing from `datasheets/`**, download it from https://www.espressif.com/en/support/documents/technical-documents and place it there before proceeding.

## PINOUT.md Format

Each `Documentation/PINOUT_<MCU>.md` file must contain:

1. **Board pinout diagram** — SuperMini board pin layout showing GPIO numbers and assigned functions.
2. **Pin function table** listing: GPIO Number, Direction, Function, Notes.
3. **Peripheral summary** — clock source, baud rate, bus configuration.
4. **Notes** — strapping pins, reserved pins, errata.

## Rules

- Pin assignments are configured via ESP-IDF Kconfig (`menuconfig`) or in component source files.
- All three targets (ESP32-C3, ESP32-H2, ESP32-C5) should use compatible pin assignments where GPIO numbering allows.
- USB pins (GPIO18/19 on C3, GPIO26/27 on C5) are reserved for USB-CDC — never reassign.
- Strapping pins (GPIO2, GPIO8, GPIO9 on C3) have boot-mode functions — document any usage carefully.
- When adding a new peripheral, create or update the corresponding ESP-IDF component.
