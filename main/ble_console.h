/**
 * @file ble_console.h
 * @brief BLE NUS (Nordic UART Service) console.
 *
 * Provides a wireless console over BLE using the Nordic UART
 * Service (NUS).  When a BLE client connects and subscribes,
 * commands received via NUS RX are dispatched through the
 * console command table.  Output from command handlers is
 * mirrored to both UART and BLE TX notifications.
 *
 * To redirect command output to BLE, include this header
 * LAST and add:
 *     #define printf console_printf
 * after all #include directives in the command source file.
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise the BLE NUS console service.
 *
 * Registers the Nordic SPP service handler with the BTstack
 * ATT server.  Must be called after the BT stack is fully
 * initialised (e.g. from on_init_complete callback).
 *
 * @sideeffects Registers BTstack service handler and HCI
 *              event callback.
 */
void ble_console_init(void);

/**
 * @brief Check whether a BLE console client is connected.
 *
 * @return true if a BLE NUS client has subscribed to TX
 *         notifications.
 */
bool ble_console_connected(void);

/**
 * @brief Send raw data to the connected BLE console client.
 *
 * Enqueues data into the TX ring buffer and requests a
 * notification slot from the ATT server.  No-op if no
 * client is connected.
 *
 * @param[in] data  Pointer to bytes to send.
 * @param[in] len   Number of bytes.
 *
 * @sideeffects Writes to BLE TX ring buffer, schedules
 *              ATT notification.
 */
void ble_console_send(const char *data, int len);

/**
 * @brief printf replacement that mirrors to BLE NUS TX.
 *
 * Formats the string, writes to stdout (UART), and if a
 * BLE console client is connected also enqueues the output
 * for BLE TX notification.
 *
 * @param[in] fmt  printf-style format string.
 * @param[in] ...  Format arguments.
 * @return Number of characters formatted (as vsnprintf).
 */
int console_printf(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

#ifdef __cplusplus
}
#endif
