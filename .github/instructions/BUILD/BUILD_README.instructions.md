---
applyTo: "**"
---

# Build Guide — Bobbycar-Steering (ESP32-H2 / ESP32-C3 / ESP32-C5)

## Tools

| Tool | Source |
|------|--------|
| ESP-IDF | `sdk/esp-idf/` (git submodule) |
| CMake + Ninja | Installed by ESP-IDF setup |
| Python 3.9+ | Required by ESP-IDF |

---

## Architecture

ESP-IDF component-based project with LVGL display support.

| Component | Description | Location |
|-----------|-------------|----------|
| **main** | Application entry point and core logic | `main/` |
| **lvgl** | LVGL graphics library (submodule) | `components/lvgl/` |
| **display** | Display driver (SPI/I2C) | `components/display/` |
| **steering** | Steering control logic | `components/steering/` |

---

## 1. First-Time Setup

```powershell
# Run the setup script (installs ESP-IDF, initialises submodules)
powershell -ExecutionPolicy Bypass -File .\setup.ps1
```

Or manually:

```powershell
# Initialise submodules
git submodule update --init --recursive

# Install ESP-IDF tools
cd sdk/esp-idf
.\install.ps1
cd ../..

# Activate ESP-IDF environment
sdk\esp-idf\export.ps1
```

---

## 2. Build

### Set Target (once per target change)

```powershell
idf.py set-target esp32c3    # ESP32-C3
idf.py set-target esp32h2    # ESP32-H2
idf.py set-target esp32c5    # ESP32-C5
```

### Build

```powershell
idf.py build
```

### Build + Flash

```powershell
idf.py -p COM<N> flash
```

### Build Outputs

| File | Location |
|------|----------|
| Firmware binary | `build/bobbycar-steering.bin` |
| Partition table | `build/partition_table/partition-table.bin` |
| Bootloader | `build/bootloader/bootloader.bin` |

---

## 3. Supported Targets

| Target | SoC | CPU | Clock | SRAM | Connectivity |
|--------|-----|-----|-------|------|-------------|
| `esp32c3` | ESP32-C3 | RISC-V single-core | 160 MHz | 400 KB | Wi-Fi 4 + BLE 5 |
| `esp32h2` | ESP32-H2 | RISC-V single-core | 96 MHz | 320 KB | BLE 5 + 802.15.4 (Zigbee/Thread) |
| `esp32c5` | ESP32-C5 | RISC-V single-core | 240 MHz | 384 KB | Wi-Fi 6 (dual-band) + BLE 5 + 802.15.4 |

---

## 4. Cross-Platform Compilation

All code in `main/` must compile for all three targets. Use Kconfig and preprocessor guards:

```c
#if CONFIG_IDF_TARGET_ESP32C3
    // C3-specific code (Wi-Fi available)
#elif CONFIG_IDF_TARGET_ESP32H2
    // H2-specific code (no Wi-Fi, has 802.15.4)
#elif CONFIG_IDF_TARGET_ESP32C5
    // C5-specific code (Wi-Fi 6 dual-band)
#endif
```

---

## 5. Common Build Errors

**ESP-IDF not found** — Run `sdk\esp-idf\export.ps1` to set `IDF_PATH` and activate the environment.

**Wrong target** — Run `idf.py set-target <target>` then `idf.py build`. Changing target requires `idf.py fullclean` first.

**Component not found** — Ensure LVGL submodule is initialised: `git submodule update --init --recursive`.

**sdkconfig conflict** — After changing target, delete `sdkconfig` and rebuild. Target defaults are in `sdkconfig.defaults` and `sdkconfig.defaults.<target>`.
