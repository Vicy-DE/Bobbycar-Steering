# Todo: ADC Display

**Created:** 2026-03-27
**Requirement:** [Req #7 — ADC Analog Sensor Readout](../Requirements/requirements.md), [Req #8 — 3.5" SPI TFT Display](../Requirements/requirements.md)
**Status:** Build OK — Pending Hardware Verification

---

## Tasks

- [x] Update requirements.md with Req #7 and #8
- [x] Create pin assignments per target (`main/pin_config.h`)
- [x] Create pinout documentation for each board
  - [x] `Documentation/PINOUT_ESP32C3.md`
  - [x] `Documentation/PINOUT_ESP32H2.md`
  - [x] `Documentation/PINOUT_ESP32C5.md`
- [x] Update datasheets/README.md with display info
- [x] Create display component (`components/display/`)
  - [x] `components/display/CMakeLists.txt`
  - [x] `components/display/include/display.h`
  - [x] `components/display/display.c` — ST7796S SPI driver + LVGL
- [x] Update `main/main.c` — ADC reading + LVGL UI
- [x] Update `main/CMakeLists.txt` — add dependencies
- [x] Update `sdkconfig.defaults` — LVGL and display config
- [x] Build passes with feature integrated (`idf.py build`)
- [ ] Flashed and verified on hardware (`idf.py flash monitor`)
- [x] CHANGE_LOG.md updated
- [x] requirements.md updated (if scope changed)

---

## Notes

- Display module has no built-in touch. I2C pins reserved for future external touch controller.
- ST7796S uses SPI mode 0, supports up to 40 MHz clock.
- LVGL v9.2 uses new display driver API (`lv_display_create()`, `lv_display_set_flush_cb()`).
- ESP32-H2 has fewer GPIOs — pin assignments differ from C3/C5.
- Byte swapping needed for RGB565 (little-endian RISC-V → big-endian SPI).
