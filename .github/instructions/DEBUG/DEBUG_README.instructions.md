---
applyTo: "**"
---

# Debug Guide — Bobbycar-Steering (ESP32-H2 / ESP32-C3 / ESP32-C5)

## Tools

| Tool | Source |
|------|--------|
| OpenOCD | Bundled with ESP-IDF (`$IDF_PATH/tools/openocd-esp32/`) |
| GDB | `riscv32-esp-elf-gdb` (installed by ESP-IDF) |
| Serial Monitor | `idf.py monitor` (via pyserial) |

Probe: **USB-CDC** built into ESP32 SuperMini boards (no external debugger needed for flashing).
JTAG: Available via USB on ESP32-C3/C5 (built-in USB-JTAG), ESP32-H2 requires external JTAG.

---

## 1. Build

See [BUILD/BUILD_README.instructions.md](../BUILD/BUILD_README.instructions.md).

```powershell
idf.py build
```

---

## 2. Flash (USB-CDC — Most Common)

```powershell
idf.py -p COM<N> flash
```

ESP32 SuperMini boards use USB-CDC for flashing. Hold BOOT button + press RST to enter download mode if auto-reset fails.

---

## 3. Flash (JTAG — ESP32-C3/C5 Built-In USB-JTAG)

```powershell
idf.py -p COM<N> -b 460800 flash
```

Or via OpenOCD:

```powershell
openocd -f board/esp32c3-builtin.cfg -c "program_esp build/bobbycar-steering.bin 0x10000 verify exit"
```

---

## 4. VS Code Debug (Recommended for Interactive Debugging)

Press **F5** with the appropriate launch configuration.

### What Happens Automatically

1. Build task runs (`idf.py build`)
2. OpenOCD starts with ESP32 config
3. GDB connects to OpenOCD on `localhost:3333`
4. Firmware is flashed
5. CPU halts at `app_main()`

---

## 5. Manual GDB Session

### Start OpenOCD (Terminal 1)

```powershell
openocd -f board/esp32c3-builtin.cfg
```

### Connect GDB (Terminal 2)

```powershell
riscv32-esp-elf-gdb build/bobbycar-steering.elf
```

```gdb
set remotetimeout unlimited
target extended-remote localhost:3333
monitor reset halt
load
monitor reset halt
break app_main
continue
```

---

## 6. Serial Monitor

```powershell
idf.py -p COM<N> monitor
```

Exit with `Ctrl+]`.

### Expected Application Output

```
I (xxx) main: Bobbycar-Steering v1.0
I (xxx) main: Target: ESP32-C3
I (xxx) steering: Initializing steering controller...
I (xxx) display: LVGL display initialized
```

### Monitoring on Windows (PowerShell)

```powershell
# Option 1: idf.py monitor (recommended — decodes crash backtraces)
idf.py -p COM<N> monitor

# Option 2: Python miniterm
py -3 -m serial.tools.miniterm COM<N> 115200
```

---

## 7. ESP32 SuperMini Board Pin Reference

### USB-CDC (Built-In)

All ESP32 SuperMini boards have USB Type-C with built-in USB-CDC for programming and serial output.

| Function | Details |
|----------|---------|
| Flash | Hold BOOT + press RST to enter download mode |
| Serial Monitor | Auto-detected COM port at 115200 baud |
| JTAG | Built-in USB-JTAG on ESP32-C3 and ESP32-C5 |

---

## 8. Troubleshooting

**Board not detected** — Install CP210x or CH340 driver if the SuperMini uses an external USB-UART chip. ESP32-C3/C5 with native USB need no driver.

**Flash fails** — Hold BOOT button, press RST, release BOOT. Then run `idf.py flash`.

**Monitor shows garbage** — Verify baud rate is 115200. Some bootloader messages use 74880 baud.

**OpenOCD fails** — Kill existing instances: `Get-Process openocd | Stop-Process -Force`. Ensure only one tool accesses the USB-JTAG interface.

**Crash backtrace** — Use `idf.py monitor` which auto-decodes RISC-V backtraces from `mcause`/`mepc` registers.
