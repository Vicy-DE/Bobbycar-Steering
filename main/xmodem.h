/**
 * @file xmodem.h
 * @brief XMODEM-CRC protocol for UART file transfer.
 *
 * Implements XMODEM with CRC-16 (polynomial 0x1021) for
 * sending and receiving files over the serial console.
 * Supports both 128-byte (SOH) and 1024-byte (STX) packets.
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* Protocol constants */
#define XMODEM_SOH       0x01  /**< 128-byte packet.    */
#define XMODEM_STX       0x02  /**< 1024-byte packet.   */
#define XMODEM_EOT       0x04  /**< End of transmission.*/
#define XMODEM_ACK       0x06  /**< Acknowledge.        */
#define XMODEM_NAK       0x15  /**< Not acknowledge.    */
#define XMODEM_CAN       0x18  /**< Cancel.             */
#define XMODEM_CRC_CHAR  'C'   /**< Request CRC mode.   */

/* Error codes */
#define XMODEM_ERR_TIMEOUT   (-1)
#define XMODEM_ERR_CANCEL    (-2)
#define XMODEM_ERR_SYNC      (-3)
#define XMODEM_ERR_CRC       (-4)
#define XMODEM_ERR_SEQ       (-5)
#define XMODEM_ERR_IO        (-6)
#define XMODEM_ERR_OVERFLOW  (-7)
#define XMODEM_ERR_RETRIES   (-8)

/** Maximum retries per packet. */
#define XMODEM_MAX_RETRIES   10

/** Timeout waiting for data (milliseconds). */
#define XMODEM_TIMEOUT_MS    3000

/** XMODEM 128-byte data payload. */
#define XMODEM_BLOCK_128     128

/** XMODEM 1024-byte data payload. */
#define XMODEM_BLOCK_1K      1024

/**
 * @brief Compute CRC-16/XMODEM.
 *
 * Polynomial 0x1021, initial value 0x0000.
 *
 * @param[in] data  Input bytes.
 * @param[in] len   Number of bytes.
 * @return CRC-16 value.
 */
uint16_t crc16_xmodem(const uint8_t *data, uint32_t len);

/**
 * @brief Receive a file via XMODEM-CRC.
 *
 * Writes received data to @p buf up to @p buf_size bytes.
 *
 * @param[out] buf       Destination buffer.
 * @param[in]  buf_size  Buffer capacity.
 * @return Bytes received (>=0) or XMODEM_ERR_* on failure.
 *
 * @sideeffects Reads/writes UART via getchar/putchar.
 */
long xmodem_receive(uint8_t *buf, size_t buf_size);

/**
 * @brief Send data via XMODEM-CRC.
 *
 * Sends @p size bytes from @p buf.  Automatically selects
 * 1K packets when size > 1024.
 *
 * @param[in] buf   Source data.
 * @param[in] size  Number of bytes to send.
 * @return Bytes sent (>=0) or XMODEM_ERR_* on failure.
 *
 * @sideeffects Reads/writes UART via getchar/putchar.
 */
long xmodem_send(const uint8_t *buf, size_t size);
