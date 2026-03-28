/**
 * @file xmodem.c
 * @brief XMODEM-CRC protocol implementation.
 *
 * Ported from the 1180 bootloader XMODEM module.  Uses
 * getchar()/putchar() for UART I/O, with FreeRTOS delays
 * for timeout handling.
 */

#include "xmodem.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ------------------------------------------------------ */
/*  CRC-16 / XMODEM                                      */
/* ------------------------------------------------------ */

/* See xmodem.h for doc comment. */
uint16_t crc16_xmodem(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0x0000;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* ------------------------------------------------------ */
/*  Low-level I/O helpers                                 */
/* ------------------------------------------------------ */

/**
 * @brief Send one byte.
 *
 * @param[in] c  Byte to send.
 *
 * @sideeffects Writes to stdout.
 */
static void xm_putc(uint8_t c)
{
    putchar((int)c);
    fflush(stdout);
}

/**
 * @brief Try to read one byte with timeout.
 *
 * @param[out] out     Received byte.
 * @param[in]  tmo_ms  Timeout in milliseconds.
 * @return true if byte read, false on timeout.
 *
 * @sideeffects Reads from stdin, may delay.
 */
static bool xm_getc(uint8_t *out, int tmo_ms)
{
    TickType_t end = xTaskGetTickCount()
        + pdMS_TO_TICKS(tmo_ms);
    while (1) {
        int c = getchar();
        if (c != EOF) {
            *out = (uint8_t)c;
            return true;
        }
        if (xTaskGetTickCount() >= end) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief Flush any pending input bytes.
 *
 * @sideeffects Reads and discards from stdin.
 */
static void xm_flush_input(void)
{
    int c;
    do {
        c = getchar();
    } while (c != EOF);
}

/* ------------------------------------------------------ */
/*  Receive                                               */
/* ------------------------------------------------------ */

/* See xmodem.h for doc comment. */
long xmodem_receive(uint8_t *buf, size_t buf_size)
{
    uint8_t  seq_expected = 1;
    size_t   total = 0;
    int      retries = 0;
    bool     started = false;

    xm_flush_input();

    while (1) {
        /* Send 'C' to request CRC mode until first packet. */
        if (!started) {
            xm_putc(XMODEM_CRC_CHAR);
        }

        uint8_t header;
        if (!xm_getc(&header, XMODEM_TIMEOUT_MS)) {
            if (++retries > XMODEM_MAX_RETRIES) {
                return XMODEM_ERR_TIMEOUT;
            }
            continue;
        }

        if (header == XMODEM_EOT) {
            xm_putc(XMODEM_ACK);
            return (long)total;
        }

        if (header == XMODEM_CAN) {
            uint8_t c2;
            if (xm_getc(&c2, 500) && c2 == XMODEM_CAN) {
                return XMODEM_ERR_CANCEL;
            }
            continue;
        }

        int blk_size;
        if (header == XMODEM_SOH) {
            blk_size = XMODEM_BLOCK_128;
        } else if (header == XMODEM_STX) {
            blk_size = XMODEM_BLOCK_1K;
        } else {
            continue;  /* garbage — retry */
        }

        started = true;

        /* Read seq, ~seq, data[blk_size], crc_h, crc_l */
        uint8_t pkt[1024 + 4];
        bool ok = true;
        for (int i = 0; i < blk_size + 4; i++) {
            if (!xm_getc(&pkt[i], XMODEM_TIMEOUT_MS)) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            xm_putc(XMODEM_NAK);
            retries++;
            if (retries > XMODEM_MAX_RETRIES) {
                return XMODEM_ERR_RETRIES;
            }
            continue;
        }

        uint8_t seq   = pkt[0];
        uint8_t seq_c = pkt[1];
        if ((seq + seq_c) != 0xFF) {
            xm_putc(XMODEM_NAK);
            continue;
        }

        /* Verify CRC-16 over data block. */
        uint16_t crc_rx = ((uint16_t)pkt[2 + blk_size] << 8)
                        | pkt[3 + blk_size];
        uint16_t crc_calc = crc16_xmodem(
            &pkt[2], (uint32_t)blk_size);
        if (crc_rx != crc_calc) {
            xm_putc(XMODEM_NAK);
            continue;
        }

        /* Check sequence. */
        if (seq != seq_expected) {
            if (seq == (uint8_t)(seq_expected - 1)) {
                /* Duplicate — ACK but don't store. */
                xm_putc(XMODEM_ACK);
                continue;
            }
            xm_putc(XMODEM_CAN);
            xm_putc(XMODEM_CAN);
            return XMODEM_ERR_SEQ;
        }

        /* Copy data to output buffer. */
        size_t copy_len = (size_t)blk_size;
        if (total + copy_len > buf_size) {
            copy_len = buf_size - total;
        }
        if (copy_len > 0) {
            memcpy(buf + total, &pkt[2], copy_len);
        }
        total += copy_len;

        seq_expected++;
        retries = 0;
        xm_putc(XMODEM_ACK);

        if (total >= buf_size) {
            return XMODEM_ERR_OVERFLOW;
        }
    }
}

/* ------------------------------------------------------ */
/*  Send                                                  */
/* ------------------------------------------------------ */

/* See xmodem.h for doc comment. */
long xmodem_send(const uint8_t *buf, size_t size)
{
    uint8_t seq = 1;
    size_t  offset = 0;
    int     retries = 0;

    /* Wait for receiver's 'C' (CRC mode request). */
    while (1) {
        uint8_t c;
        if (!xm_getc(&c, XMODEM_TIMEOUT_MS)) {
            if (++retries > XMODEM_MAX_RETRIES) {
                return XMODEM_ERR_TIMEOUT;
            }
            continue;
        }
        if (c == XMODEM_CRC_CHAR) {
            break;
        }
        if (c == XMODEM_CAN) {
            return XMODEM_ERR_CANCEL;
        }
    }

    bool use_1k = (size > 1024);

    while (offset < size) {
        int blk_size = use_1k
            ? XMODEM_BLOCK_1K : XMODEM_BLOCK_128;
        size_t remain = size - offset;
        if ((size_t)blk_size > remain && !use_1k) {
            blk_size = XMODEM_BLOCK_128;
        }

        /* Build packet. */
        uint8_t pkt[1 + 2 + 1024 + 2];
        pkt[0] = (blk_size == XMODEM_BLOCK_1K)
                 ? XMODEM_STX : XMODEM_SOH;
        pkt[1] = seq;
        pkt[2] = ~seq;

        size_t copy = remain < (size_t)blk_size
                    ? remain : (size_t)blk_size;
        memcpy(&pkt[3], buf + offset, copy);
        /* Pad remainder with 0x1A (EOF filler). */
        if (copy < (size_t)blk_size) {
            memset(&pkt[3 + copy], 0x1A,
                   (size_t)blk_size - copy);
        }

        uint16_t crc = crc16_xmodem(
            &pkt[3], (uint32_t)blk_size);
        pkt[3 + blk_size]     = (crc >> 8) & 0xFF;
        pkt[3 + blk_size + 1] = crc & 0xFF;

        /* Transmit packet. */
        int pkt_len = 3 + blk_size + 2;
        for (int i = 0; i < pkt_len; i++) {
            xm_putc(pkt[i]);
        }

        /* Wait for ACK/NAK. */
        uint8_t resp;
        if (!xm_getc(&resp, XMODEM_TIMEOUT_MS)) {
            if (++retries > XMODEM_MAX_RETRIES) {
                return XMODEM_ERR_TIMEOUT;
            }
            continue;
        }
        if (resp == XMODEM_ACK) {
            offset += (size_t)blk_size;
            seq++;
            retries = 0;
        } else if (resp == XMODEM_CAN) {
            return XMODEM_ERR_CANCEL;
        } else {
            /* NAK or garbage — retry. */
            if (++retries > XMODEM_MAX_RETRIES) {
                return XMODEM_ERR_RETRIES;
            }
        }
    }

    /* Send EOT. */
    retries = 0;
    while (1) {
        xm_putc(XMODEM_EOT);
        uint8_t c;
        if (xm_getc(&c, XMODEM_TIMEOUT_MS)
            && c == XMODEM_ACK) {
            break;
        }
        if (++retries > XMODEM_MAX_RETRIES) {
            return XMODEM_ERR_TIMEOUT;
        }
    }

    return (long)offset;
}
