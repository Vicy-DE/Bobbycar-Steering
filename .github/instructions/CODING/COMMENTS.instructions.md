---
applyTo: "**/*.c,**/*.h"
---

# Coding Conventions — Function Comments & Side Effects

## RULE: Every Function Must Have a Documentation Comment

Every function — public or static — MUST have a Doxygen-style documentation comment immediately above the definition. Use the Google C/C++ Style Guide format:

```c
/**
 * @brief One-line summary ending with a period.
 *
 * Optional longer description explaining behaviour, edge cases, or
 * algorithm notes.  Wrap at 80 columns.
 *
 * @param[in]     name   Description of input parameter.
 * @param[out]    name   Description of output parameter.
 * @param[in,out] name   Description of input/output parameter.
 * @return Description of return value.  Use "void" implicitly (omit @return).
 *
 * @sideeffects Modifies global `tick_ms`.
 *              Writes to USART1 TX register.
 */
```

### Comment Checklist

1. `@brief` — mandatory, one sentence, imperative mood ("Compute …", not "Computes …").
2. `@param` — one per parameter, tagged `[in]`, `[out]`, or `[in,out]`.
3. `@return` — describe what the return value means (omit for `void`).
4. `@sideeffects` — mandatory if the function is *not* side-effect free (see below).

---

## RULE: Functions Must Be Side-Effect Free Unless Tagged `_sideeffects`

A **side-effect free** function:
- Does NOT modify global / static / file-scope variables.
- Does NOT write to hardware registers (GPIO, UART, Flash, SysTick, etc.).
- Does NOT perform I/O (serial, flash, memory-mapped registers).
- May only read its parameters and return a value (+ use local variables).

### Naming Convention

| Function type | Naming rule | Example |
|---------------|-------------|---------|
| Side-effect free | Normal name | `crc16_xmodem()`, `angle_to_pwm()` |
| Has side effects | Append `_sideeffects` to the name **OR** the function is a well-known platform API where the name already implies side effects | `steering_init()`, `display_update()` |

### Well-Known Side-Effect Exceptions

The following function name patterns are inherently understood to have side effects and do **not** need the `_sideeffects` suffix:

- `*_init`, `*_deinit` — initialisation / teardown
- `*_Handler` — interrupt handlers
- `app_main` — entry point
- `*_task` — FreeRTOS task functions
- `*_cb`, `*_callback` — callback functions
- ESP-IDF API wrappers (`esp_*`, `gpio_*`, `i2c_*`, `spi_*`, etc.)

Any **other** function with side effects that does not match the patterns above **MUST** be named with a `_sideeffects` suffix.

### When to Use `@sideeffects`

Even for well-known exception names, the `@sideeffects` tag in the doc comment is **always required** when side effects exist.

```c
/**
 * @brief Send a null-terminated string over UART.
 *
 * @param[in] s  Null-terminated string to transmit.
 *
 * @sideeffects Writes bytes to UART TX via ESP-IDF driver.
 */
static void uart_puts(const char *s)
```

---

## RULE: Header-File Declarations

Header files declare the public API. Each declaration MUST have a doc comment. The full `@param`/`@return`/`@sideeffects` block goes in the **header**, not repeated in the `.c` definition:

```c
/* In .h */
/**
 * @brief Set the steering angle.
 *
 * @param[in] angle_deg  Steering angle in degrees (-45 to +45).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of range.
 *
 * @sideeffects Writes to PWM/LEDC peripheral registers.
 */
esp_err_t steering_set_angle(float angle_deg);
```

In the `.c` file, a shorter comment referencing the header is acceptable:

```c
/* See steering.h for full documentation. */
esp_err_t steering_set_angle(float angle_deg)
{
    ...
}
```

---

## Reference

- [Google C++ Style Guide — Comments](https://google.github.io/styleguide/cppguide.html#Comments)
- Doxygen `@brief`, `@param`, `@return`: <https://www.doxygen.nl/manual/commands.html>
