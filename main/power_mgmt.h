/**
 * @file power_mgmt.h
 * @brief ESP32 power management (energy saving).
 *
 * Configures dynamic frequency scaling (DFS) and automatic
 * light sleep so the CPU enters low-power wait-for-interrupt
 * (WFI) states when idle — the RISC-V equivalent of ARM WFI.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable power management and automatic light sleep.
 *
 * Configures DFS to scale the CPU frequency down when idle
 * and enables FreeRTOS tickless idle for automatic light
 * sleep entry.  The CPU wakes on any interrupt (UART, BLE,
 * GPIO, timer).
 *
 * @sideeffects Modifies ESP32 power management config,
 *              enables tickless idle.
 */
void power_mgmt_init(void);

#ifdef __cplusplus
}
#endif
