---
applyTo: "**/*.c,**/*.h,**/*.S,**/CMakeLists.txt,**/*.bat,**/*.ps1,**/*.py,**/*.sh"
---

# Test Generation & Documentation — Instructions for Copilot

## When to execute

**After every implementation — before closing the task.**
Generate and run tests that fully cover the new or modified code. Document all executed tests in `Documentation/Tests/`.

---

## Step 1 — Generate tests

### Where to put test scripts
- Python integration / hardware tests → `Target/test_<feature>.py`
- PowerShell orchestration / CI tests → `Target/test_<feature>.ps1`

### What to test — MANDATORY coverage

| Scope | What to verify |
|---|---|
| **Happy path** | Nominal input → expected output / behaviour on hardware |
| **Error paths** | Invalid input, timeouts, peripheral failures |
| **Boundary conditions** | Min/max values, edge cases |
| **Regression** | Previously passing tests still pass after the change |
| **Platform coverage** | Run on all affected platforms (esp32c3, esp32h2, esp32c5) |

### Test script conventions
- Follow the same style as existing `Target/test_*.py` scripts.
- Each test prints `PASS` or `FAIL` with a short reason on the same line.
- Exit with code `0` on full pass, non-zero on any failure.
- Use `serial.Serial` for UART communication.
- Add a `--port` / `--baud` argument for serial scripts.

### Build the firmware under test first
```powershell
idf.py build
```
Fix all build errors before running any test.

---

## Step 2 — Execute tests

Run each test script and capture the output:

```powershell
py -3 Target/test_<feature>.py --port COM3 --baud 115200 2>&1 | Tee-Object -FilePath "Documentation/Tests/<YYYY-MM-DD>_<feature>_results.txt"
```

- **All tests MUST pass** before the task is considered done.
- If a test fails, fix the implementation, rebuild, and re-run.

---

## Step 3 — Document the tests

Create (or update) a test report in `Documentation/Tests/`.

### File naming

```
Documentation/Tests/<YYYY-MM-DD>_<feature>.md
```

### Report format

```markdown
# Test Report — <Feature / Change Title>

**Date:** YYYY-MM-DD
**IDF Target:** `esp32c3` / `esp32h2` / `esp32c5`
**ESP-IDF Version:** v5.x
**Firmware build:** <git short hash or build timestamp>

---

## Summary

| Result | Count |
|---|---|
| PASS | N |
| FAIL | 0 |

---

## Test Cases

### TC-01 — <Short test name>

**Script:** `Target/test_<feature>.py`
**Input / stimulus:** <what was sent or triggered>
**Expected result:** <what should happen>
**Actual result:** PASS / FAIL — <one-line observation>

---

## Raw output

\`\`\`
<paste or summarise critical lines from the captured terminal output>
\`\`\`

---

## Remarks

<Any anomalies, skipped tests with justification, or known limitations.>
```

---

## Rules

- **MUST** write at least one test for every new function, state transition, or protocol message.
- **MUST** run tests on real hardware (or clearly mark as simulation-only with a justification).
- **MUST** save the test report before committing.
- **MUST NOT** copy-paste "expected" output as "actual" output without running the test.
- **SHOULD** reuse existing test infrastructure.
