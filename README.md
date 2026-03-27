# Bobbycar-Steering

Cross-platform steering controller firmware for the Bobbycar project, targeting ESP32 SuperMini development boards.

## Supported Hardware

| Board | SoC | CPU | Clock | SRAM | Connectivity |
|-------|-----|-----|-------|------|-------------|
| ESP32-C3 SuperMini | ESP32-C3 | RISC-V single-core | 160 MHz | 400 KB | Wi-Fi 4 + BLE 5 |
| ESP32-H2 SuperMini | ESP32-H2 | RISC-V single-core | 96 MHz | 320 KB | BLE 5 + 802.15.4 (Zigbee/Thread) |
| ESP32-C5 SuperMini | ESP32-C5 | RISC-V single-core | 240 MHz | 384 KB | Wi-Fi 6 (2.4+5 GHz) + BLE 5 + 802.15.4 |

**Source:** [AliExpress — ESP32 SuperMini Dev Boards](https://de.aliexpress.com/item/1005009495310442.html)

## Features

- LVGL-based display output (SPI/I2C)
- Steering control via PWM/servo
- Cross-platform: single codebase for ESP32-C3, ESP32-H2, ESP32-C5
- ESP-IDF v5.x based (FreeRTOS, component architecture)

## Quick Start

```powershell
# 1. Clone with submodules
git clone --recursive <repo-url>
cd Bobbycar-Steering

# 2. Run setup (installs ESP-IDF tools)
powershell -ExecutionPolicy Bypass -File .\setup.ps1

# 3. Activate ESP-IDF environment
sdk\esp-idf\export.ps1

# 4. Set target and build
idf.py set-target esp32c3
idf.py build

# 5. Flash and monitor
idf.py -p COM<N> flash monitor
```

## Project Structure

```
Bobbycar-Steering/
├── .github/
│   ├── hooks/                      # Copilot hooks
│   └── instructions/               # Copilot workflow instructions
├── .vscode/                        # VS Code settings and tasks
├── components/
│   ├── lvgl/                       # LVGL library (git submodule)
│   ├── display/                    # Display driver component
│   └── steering/                   # Steering control component
├── datasheets/                     # ESP32 datasheets and reference manuals
├── Documentation/
│   ├── CHANGE_LOG.md               # Change history
│   ├── PROJECT_DOC.md              # Project documentation
│   ├── PINOUT_ESP32C3.md           # ESP32-C3 pin assignments
│   ├── PINOUT_ESP32H2.md           # ESP32-H2 pin assignments
│   ├── PINOUT_ESP32C5.md           # ESP32-C5 pin assignments
│   ├── Requirements/               # Feature requirements
│   ├── Tests/                      # Test reports
│   └── ToDo/                       # Feature todo lists
├── main/                           # Main application component
│   ├── CMakeLists.txt
│   └── main.c
├── sdk/
│   └── esp-idf/                    # ESP-IDF SDK (git submodule)
├── Target/                         # Test and debug scripts
├── tools/                          # Build tools and utilities
├── CMakeLists.txt                  # Root CMake (ESP-IDF project)
├── sdkconfig.defaults              # Default ESP-IDF configuration
├── setup.ps1                       # One-click setup script
└── README.md
```

## Documentation

- [Project Documentation](Documentation/PROJECT_DOC.md)
- [Change Log](Documentation/CHANGE_LOG.md)
- [Requirements](Documentation/Requirements/requirements.md)

## ESP32 SoC Comparison

| Feature | ESP32-C3 | ESP32-H2 | ESP32-C5 |
|---------|----------|----------|----------|
| Core | RISC-V 32-bit | RISC-V 32-bit | RISC-V 32-bit |
| Max Clock | 160 MHz | 96 MHz | 240 MHz |
| SRAM | 400 KB | 320 KB | 384 KB |
| Flash (SiP) | 4 MB | 2/4 MB | External |
| Wi-Fi | 802.11 b/g/n | — | 802.11 a/b/g/n/ac/ax (Wi-Fi 6) |
| Bluetooth | BLE 5 | BLE 5 | BLE 5 |
| 802.15.4 | — | Zigbee / Thread | Zigbee / Thread |
| GPIOs | 22 | 19 | 29 |
| ADC | 2× 12-bit | 1× 12-bit | 1× 12-bit |
| USB | USB-Serial/JTAG | USB-Serial/JTAG | USB-Serial/JTAG |
| Security | Secure Boot, Flash Encryption | Secure Boot, Flash Encryption, ECDSA | Secure Boot, Flash/PSRAM Encryption, TEE |
| ESP-IDF | Supported | Supported | Supported |
