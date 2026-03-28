# Requirements — Bobbycar-Steering

**Last updated:** 2026-03-28

---

## 1. Cross-Platform ESP32 Support — ESP-IDF

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | N/A |
| **Platform** | All (ESP32-C3, ESP32-H2, ESP32-C5) |
| **Requirements** | <ul><li>Firmware must compile and run on all three target SoCs: ESP32-C3, ESP32-H2, ESP32-C5</li><li>Use ESP-IDF v5.x as the SDK (git submodule at `sdk/esp-idf/`)</li><li>Target-specific code must be guarded with `#if CONFIG_IDF_TARGET_*` preprocessor directives</li><li>Target selection via `idf.py set-target`</li><li>Common sdkconfig.defaults for shared settings, target-specific sdkconfig.defaults.&lt;target&gt; files for per-target config</li></ul> |

---

## 2. LVGL Display Support — SPI/I2C Display

| Item | Detail |
|---|---|
| **Module / Component** | display, lvgl |
| **Interface** | SPI / I2C |
| **Platform** | All |
| **Requirements** | <ul><li>Integrate LVGL v9.x as a git submodule under `components/lvgl/`</li><li>Provide a display driver component that supports common SPI and I2C displays (SSD1306, ST7789, etc.)</li><li>LVGL tick and flush callbacks must be implemented for ESP-IDF FreeRTOS</li><li>Display resolution and interface configurable via Kconfig menuconfig</li></ul> |

---

## 3. Steering Control — PWM Servo Output

| Item | Detail |
|---|---|
| **Module / Component** | steering |
| **Interface** | PWM (LEDC) / GPIO |
| **Platform** | All |
| **Requirements** | <ul><li>Control a steering servo via PWM output using ESP-IDF LEDC peripheral</li><li>Configurable PWM frequency (default 50 Hz for standard servos)</li><li>Configurable pulse width range (1000–2000 µs typical, adjustable)</li><li>Angle range: -45° to +45° minimum</li><li>Dead zone / center trim configurable</li></ul> |

---

## 4. USB-CDC Serial Interface — Debug Output

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | USB-CDC |
| **Platform** | All |
| **Requirements** | <ul><li>Use built-in USB-CDC for serial debug output at 115200 baud</li><li>ESP32 SuperMini boards provide USB Type-C with native USB</li><li>Boot log, runtime status, and error messages output via ESP_LOGx macros</li><li>Support `idf.py monitor` for development</li></ul> |

---

## 5. Hardware Abstraction — SuperMini Board Support

| Item | Detail |
|---|---|
| **Module / Component** | main, steering, display |
| **Interface** | GPIO |
| **Platform** | All |
| **Requirements** | <ul><li>Pin assignments must account for different GPIO counts: C3 (22 GPIOs), H2 (19 GPIOs), C5 (29 GPIOs)</li><li>Pin assignments documented in `Documentation/PINOUT_ESP32*.md`</li><li>Strapping pins (boot mode) must not be used for critical functions during boot</li><li>USB pins reserved for USB-CDC, not available for general use</li><li>Source: [AliExpress ESP32 SuperMini](https://de.aliexpress.com/item/1005009495310442.html)</li></ul> |

---

## 6. Wireless Connectivity — BLE / Wi-Fi / 802.15.4

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | BLE / Wi-Fi / 802.15.4 |
| **Platform** | Platform-dependent |
| **Requirements** | <ul><li>ESP32-C3: Wi-Fi 4 (2.4 GHz) + BLE 5 available</li><li>ESP32-H2: BLE 5 + IEEE 802.15.4 (Zigbee/Thread) available, NO Wi-Fi</li><li>ESP32-C5: Wi-Fi 6 (2.4 + 5 GHz dual-band) + BLE 5 + 802.15.4 available</li><li>Connectivity features are optional and target-dependent</li><li>Use `#if CONFIG_IDF_TARGET_*` guards for connectivity code</li></ul> |

---

## 7. ADC Analog Sensor Readout — Two Channels

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | ADC |
| **Platform** | All (ESP32-C3, ESP32-H2, ESP32-C5) |
| **Requirements** | <ul><li>Read two analog sensor inputs via ADC1 oneshot mode</li><li>ADC attenuation: 12 dB (0–3.3 V full range)</li><li>12-bit resolution (0–4095)</li><li>ESP32-C3: ADC1_CH0 (GPIO0), ADC1_CH1 (GPIO1)</li><li>ESP32-H2: ADC1_CH0 (GPIO1), ADC1_CH1 (GPIO2)</li><li>ESP32-C5: ADC1_CH0 (GPIO0), ADC1_CH1 (GPIO1)</li><li>Sample rate: ≥10 Hz per channel for display update</li><li>Values displayed on LVGL UI</li></ul> |

---

## 8. 3.5" SPI TFT Display — ST7796S 320×480

| Item | Detail |
|---|---|
| **Module / Component** | display |
| **Interface** | SPI |
| **Platform** | All |
| **Requirements** | <ul><li>Display module: TENSTAR ROBOT 3.5" SPI TFT ([AliExpress](https://de.aliexpress.com/item/1005006175220737.html))</li><li>Driver IC: ST7796S, 320×480 pixels, RGB565 (16-bit color)</li><li>SPI interface: 4-wire (MOSI, SCK, CS, DC) + RST + BL</li><li>SPI clock: 40 MHz (max supported by ST7796S)</li><li>Landscape orientation (480×320) via MADCTL rotation</li><li>No built-in touch — I2C pins reserved for external touch controller</li><li>Backlight controlled via GPIO (on/off)</li><li>LVGL v9.2 integration with partial rendering (20-line buffer)</li><li>Display the two ADC sensor values with real-time update</li></ul> |

---

---

## 9. Blinky LED Task — GPIO Output Test

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | GPIO |
| **Platform** | All (ESP32-C3, ESP32-H2, ESP32-C5) |
| **Requirements** | <ul><li>A FreeRTOS task (`blinky_task`) must cycle the onboard WS2812 addressable RGB LED through Red, Green, and Blue colors at 500 ms intervals using the `led_strip` RMT driver</li><li>The WS2812 LED GPIO is defined per-target as `BLINKY_WS2812_GPIO` (GPIO8 on all SuperMini boards)</li><li>Additional free GPIOs defined as `BLINKY_GPIO_PINS` / `BLINKY_GPIO_COUNT` are toggled as plain push-pull outputs for external LEDs</li><li>ESP32-H2 external LED GPIOs: GPIO4, GPIO5, GPIO9, GPIO14</li><li>ESP32-C3 external LED GPIO: GPIO21</li><li>ESP32-C5 external LED GPIOs: GPIO12, GPIO13, GPIO14, GPIO15</li><li>Managed component dependency: `espressif/led_strip ^2.4.1`</li><li>Task stack: 4096 bytes, priority 5</li><li>Primary purpose: hardware bring-up and RGB LED verification</li></ul> |

---

## 10. Bluetooth Gamepad Support — Bluepad32 (BLE)

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | BLE |
| **Platform** | All (ESP32-C3, ESP32-H2, ESP32-C5) |
| **Requirements** | <ul><li>Integrate Bluepad32 v4.2.0 library as a git submodule under `components/bluepad32/` for Bluetooth gamepad input</li><li>Use BTstack Bluetooth stack (bundled with Bluepad32) via the ESP32 VHCI controller interface</li><li>Support Xbox Wireless controllers: model 1914 (BLE native, firmware v5.15+), model 1708 (BLE since firmware v5.x), and Xbox Adaptive (BLE native)</li><li>ESP32-H2 / ESP32-C3 / ESP32-C5 are BLE-only — only BLE gamepads are supported (no BR/EDR / Bluetooth Classic)</li><li>Additional BLE gamepads supported: Steam Controller (with BT firmware), Stadia Controller (with BT firmware)</li><li>Custom Bluepad32 platform implementation providing callbacks: `on_device_connected`, `on_device_disconnected`, `on_device_ready`, `on_controller_data`</li><li>Gamepad data (axes, buttons) logged to serial console for verification</li><li>BTstack runs in the main task; existing sensor/display loop offloaded to a FreeRTOS task</li><li>Bluepad32 callbacks must NOT call expensive functions — use FreeRTOS queue to pass gamepad data to display task if needed</li></ul> |

---

## 11. UART Console Shell — Interactive CLI

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | UART / USB-CDC |
| **Platform** | All |
| **Requirements** | <ul><li>FreeRTOS console task reads lines from stdin (USB-CDC) at 115200 baud</li><li>Command dispatch using 1180-style `shell_cmd_t` table: `{name, description, handler(argc, argv)}`</li><li>Software design follows bobbycar-project `command_interpreter.c` dispatch pattern</li><li>Help command prints aligned name + description for all registered commands (1180 format)</li><li>Commands: echo, ver, reset, sets, gets, getds, sett, gett, getb, seti, geti, setm, getm, setkp/ki/kd, getkp/ki/kd, getpo</li><li>File commands: ls, cd, pwd, rm, mkdir, show, recv, send, format</li><li>No file > 400 lines, no line > 100 characters</li></ul> |

---

## 12. XMODEM-CRC File Transfer — UART Binary Transfer

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | UART / USB-CDC |
| **Platform** | All |
| **Requirements** | <ul><li>XMODEM-CRC (polynomial 0x1021) protocol for file transfer over UART</li><li>Supports 128-byte (SOH) and 1024-byte (STX) packets</li><li>`recv <path>` receives a file via XMODEM-CRC and saves to LittleFS</li><li>`send <path>` reads a file from LittleFS and sends via XMODEM-CRC</li><li>8 KB transfer buffer, max file size limited by LittleFS partition (960 KB)</li><li>Timeout: 3 seconds per packet, max 10 retries</li></ul> |

---

## 13. Four-Wheel Motor Objects — Bobbycar Drive

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | N/A (data model) |
| **Platform** | All |
| **Requirements** | <ul><li>Four motor objects: front-left (FL), front-right (FR), rear-left (RL), rear-right (RR)</li><li>Each motor has: torque (-1000..+1000), speed_meas, bat_voltage, board_temp, enabled, connected</li><li>Hoverboard protocol constants: START_FRAME 0x7A7A, baud 57600</li><li>Throttle limits: THROTTLE_MAX=1000, THROTTLE_REVERSE_MAX=300</li><li>Bulk torque assignment and individual get/set operations</li></ul> |

---

## 14. Ackermann Steering Algorithm — Torque Distribution

| Item | Detail |
|---|---|
| **Module / Component** | main |
| **Interface** | N/A (algorithm) |
| **Platform** | All |
| **Requirements** | <ul><li>Port of bobbycar-project `calc_torque_per_wheel()` using Ackermann geometry</li><li>Chassis: wheelbase=35cm, width=30cm, steering_width=20cm, steering_to_wheel=5cm</li><li>Computes individual torque for each of 4 wheels based on steering angle</li><li>Inner wheels receive less torque, outer wheels more during turns</li><li>Normalized so total torque output equals the throttle input × 4</li><li>PID correction applied to front-axle differential (torque[0] += regulated, torque[1] -= regulated)</li><li>Output clamped to ±THROTTLE_MAX</li></ul> |

---

## Traceability Matrix

| Req # | Feature | Depends On |
|---|---|---|
| 1 | Cross-Platform ESP32 Support | — |
| 2 | LVGL Display Support | 1 |
| 9 | Blinky LED Task | 1 |
| 3 | Steering Control | 1 |
| 4 | USB-CDC Serial Interface | 1 |
| 5 | Hardware Abstraction | 1 |
| 6 | Wireless Connectivity | 1, 5 |
| 7 | ADC Analog Sensor Readout | 1, 5 |
| 8 | 3.5" SPI TFT Display | 1, 2, 5, 7 |
| 10 | Bluetooth Gamepad Support | 1, 6 |
| 11 | UART Console Shell | 1, 4 |
| 12 | XMODEM File Transfer | 11 |
| 13 | Four-Wheel Motor Objects | 1 |
| 14 | Ackermann Steering Algorithm | 13 |
