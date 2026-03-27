---
applyTo: "**/*.c,**/*.h,**/*.S,**/CMakeLists.txt,**/*.bat,**/*.ps1,**/*.py,**/*.sh"
---

# Coding Conventions — Scripts

## RULE: Script Locations

Scripts are organized by purpose into two directories:

| Directory | Purpose | Examples |
|-----------|---------|---------|
| `Target/` | Hardware interaction — flashing, debugging, testing, serial capture | `test_steering.py`, `capture_serial.py` |
| `tools/` | Build tools — partition generation, config utilities | `generate_partition.py` |

**MUST:** Place new scripts in the appropriate directory based on their purpose.

### Exceptions (do NOT move)

| Path pattern | Reason |
|---|---|
| `sdk/**` | ESP-IDF SDK scripts — never modify or move |
| `components/lvgl/**` | LVGL library — never modify or move |

---

## RULE: Use Script-Relative Paths

When a script needs to reference files relative to the project root, use the script's own location as the anchor — **never** `os.getcwd()` or `$PWD`.

### Python

```python
import os
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.join(SCRIPT_DIR, "..")
```

### PowerShell

```powershell
$ScriptDir = Split-Path $MyInvocation.MyCommand.Path -Parent
$ProjectRoot = Split-Path $ScriptDir -Parent
```

---

## RULE: Naming Convention

- Use `snake_case` for Python scripts: `test_steering.py`, `capture_serial.py`
- Use `kebab-case` for shell scripts: `build-all.ps1`
- Exception: legacy scripts may keep their established names

---

## RULE: Never Create Temporary Scripts in the Workspace Root

Do **NOT** create one-off analysis/debug scripts in the workspace root or random directories.

- Use the terminal directly for one-off commands
- If a diagnostic script is needed, place it in `Target/` with a `diag_` prefix
- Only create a permanent script if it will be reused

---

## Target/ Inventory

| Script | Language | Purpose |
|--------|----------|---------|
| (to be created) | | |

## tools/ Inventory

| Script | Language | Purpose |
|--------|----------|---------|
| (to be created) | | |
