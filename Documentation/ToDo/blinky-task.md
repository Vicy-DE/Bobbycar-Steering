# Todo: Blinky LED Task

**Created:** 2026-03-28
**Requirement:** [Req #9 — Blinky LED Task](../Requirements/requirements.md)
**Status:** Done

---

## Tasks

- [x] Update `requirements.md` — add Req #9
- [x] Add `BLINKY_LED_PINS` and `BLINKY_LED_COUNT` to `main/pin_config.h` for all three targets
- [x] Add `#include "driver/gpio.h"` to `main/main.c`
- [x] Add `esp_driver_gpio` to `PRIV_REQUIRES` in `main/CMakeLists.txt`
- [x] Implement `blinky_task()` in `main/main.c`
  - [x] Configure all LED GPIO pins as outputs
  - [x] Cycle through each LED: ON → 500 ms delay → OFF → next
- [x] Start `blinky_task` via `xTaskCreate()` in `app_main()`
- [x] Set ESP32-H2 target (`idf.py set-target esp32h2`)
- [x] Build passes with feature integrated (`idf.py build`)
- [x] Flashed and verified on hardware (`idf.py flash monitor`)
- [x] **Fix:** GPIO8 is a WS2812 addressable RGB LED, not a simple GPIO LED
  - [x] Replace raw GPIO toggle with `led_strip` RMT driver
  - [x] Split `BLINKY_LED_PINS` into `BLINKY_WS2812_GPIO` + `BLINKY_GPIO_PINS`
  - [x] Add `main/idf_component.yml` with `espressif/led_strip ^2.4.1`
  - [x] Cycle WS2812 through R→G→B at 500 ms
  - [x] Rebuild, reflash, and verify all 3 colors visible
- [x] CHANGE_LOG.md updated
- [x] requirements.md updated (if scope changed)

---

## Notes

- Primary target for verification: ESP32-H2 SuperMini
- **GPIO8 is a WS2812 addressable RGB LED** — requires RMT one-wire protocol, not plain GPIO
- Raw GPIO toggling on GPIO8 causes only green to flash randomly (WS2812 latches garbage data)
- ESP32-H2 external GPIOs (plain toggle): GPIO4, GPIO5, GPIO9, GPIO14
- GPIO9 is a strapping pin — safe for output after boot
- GPIO2/GPIO3 are reserved for TWAI RX/TX; GPIO1/GPIO2 for ADC sensors
- All three targets use GPIO8 for WS2812 (SuperMini board commonality)
