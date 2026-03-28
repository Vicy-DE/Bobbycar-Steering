/**
 * @file ble_console.c
 * @brief BLE NUS console — Nordic UART Service over BTstack.
 *
 * Uses the BTstack nordic_spp_service_server to provide a
 * wireless serial console.  Received data is accumulated
 * into a line buffer and dispatched via console_exec().
 * The console_printf() function mirrors output to both
 * UART and BLE TX notifications.
 */

#include "ble_console.h"
#include "console.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "btstack.h"
#include "ble/att_server.h"
#include "ble/gatt-service/nordic_spp_service_server.h"

static const char *TAG = "ble_con";

/* ------------------------------------------------------ */
/*  TX ring buffer                                        */
/* ------------------------------------------------------ */

#define BLE_TX_BUF_SIZE 1024

static char           tx_buf[BLE_TX_BUF_SIZE];
static volatile int   tx_head;
static volatile int   tx_tail;
static SemaphoreHandle_t tx_mux;

/**
 * @brief Number of bytes waiting in the TX ring buffer.
 *
 * @return Byte count available to read.
 */
static int tx_available(void)
{
    int a = tx_head - tx_tail;
    if (a < 0) {
        a += BLE_TX_BUF_SIZE;
    }
    return a;
}

/**
 * @brief Enqueue bytes into the TX ring buffer.
 *
 * @param[in] data  Source bytes.
 * @param[in] len   Number of bytes to enqueue.
 *
 * @sideeffects Modifies tx_buf, tx_head.
 */
static void tx_put(const char *data, int len)
{
    xSemaphoreTake(tx_mux, portMAX_DELAY);
    for (int i = 0; i < len; i++) {
        int next = (tx_head + 1) % BLE_TX_BUF_SIZE;
        if (next == tx_tail) {
            break;  /* buffer full — drop oldest is risky,
                       just stop filling. */
        }
        tx_buf[tx_head] = data[i];
        tx_head = next;
    }
    xSemaphoreGive(tx_mux);
}

/**
 * @brief Dequeue bytes from the TX ring buffer.
 *
 * @param[out] data   Destination buffer.
 * @param[in]  maxlen Maximum bytes to dequeue.
 * @return Actual bytes dequeued.
 *
 * @sideeffects Modifies tx_tail.
 */
static int tx_get(char *data, int maxlen)
{
    int n = 0;
    xSemaphoreTake(tx_mux, portMAX_DELAY);
    while (n < maxlen && tx_tail != tx_head) {
        data[n++] = tx_buf[tx_tail];
        tx_tail = (tx_tail + 1) % BLE_TX_BUF_SIZE;
    }
    xSemaphoreGive(tx_mux);
    return n;
}

/* ------------------------------------------------------ */
/*  RX line buffer                                        */
/* ------------------------------------------------------ */

#define BLE_RX_LINE_SIZE CONSOLE_LINE_MAX

static char rx_line[BLE_RX_LINE_SIZE];
static int  rx_pos;

/* ------------------------------------------------------ */
/*  Connection state                                      */
/* ------------------------------------------------------ */

static hci_con_handle_t ble_con_handle = HCI_CON_HANDLE_INVALID;
static bool             ble_is_connected;

static btstack_context_callback_registration_t send_request;

/* ------------------------------------------------------ */
/*  ATT notification (TX) callback                        */
/* ------------------------------------------------------ */

/**
 * @brief Called by BTstack when a notification slot is available.
 *
 * Sends the next chunk from the TX ring buffer to the BLE
 * client.  Requests another slot if data remains.
 *
 * @param[in] context  Unused.
 *
 * @sideeffects Sends ATT notification, may re-register
 *              callback.
 */
static void ble_can_send_cb(void *context)
{
    (void)context;
    if (ble_con_handle == HCI_CON_HANDLE_INVALID) {
        return;
    }

    uint16_t mtu = att_server_get_mtu(ble_con_handle);
    if (mtu < 3) {
        return;
    }
    uint16_t max_chunk = mtu - 3;  /* ATT header overhead */
    if (max_chunk > 240) {
        max_chunk = 240;
    }

    char chunk[240];
    int n = tx_get(chunk, max_chunk);
    if (n > 0) {
        nordic_spp_service_server_send(ble_con_handle,
                                       (const uint8_t *)chunk,
                                       (uint16_t)n);
        if (tx_available() > 0) {
            nordic_spp_service_server_request_can_send_now(
                &send_request, ble_con_handle);
        }
    }
}

/* ------------------------------------------------------ */
/*  NUS packet handler                                    */
/* ------------------------------------------------------ */

/**
 * @brief BTstack packet handler for NUS events and data.
 *
 * Handles connect/disconnect meta-events and incoming
 * data packets (RFCOMM_DATA_PACKET as used by the nordic
 * SPP service server implementation).
 *
 * @param[in] packet_type  HCI_EVENT_PACKET or RFCOMM_DATA_PACKET.
 * @param[in] channel      Connection handle for data packets.
 * @param[in] packet       Event or data payload.
 * @param[in] size         Payload length.
 *
 * @sideeffects Modifies connection state, calls
 *              console_exec().
 */
static void ble_nus_handler(uint8_t packet_type,
                             uint16_t channel,
                             uint8_t *packet,
                             uint16_t size)
{
    /* ---------- Received data from BLE client ---------- */
    if (packet_type == RFCOMM_DATA_PACKET) {
        for (uint16_t i = 0; i < size; i++) {
            char c = (char)packet[i];
            if (c == '\r' || c == '\n') {
                if (rx_pos > 0) {
                    rx_line[rx_pos] = '\0';
                    ESP_LOGI(TAG, "BLE cmd: %s", rx_line);
                    console_exec(rx_line);
                    /* Send prompt back to BLE client. */
                    const char *prompt = "\r\n> ";
                    tx_put(prompt, (int)strlen(prompt));
                    if (ble_con_handle != HCI_CON_HANDLE_INVALID) {
                        nordic_spp_service_server_request_can_send_now(
                            &send_request, ble_con_handle);
                    }
                    rx_pos = 0;
                }
            } else if (rx_pos < BLE_RX_LINE_SIZE - 1) {
                rx_line[rx_pos++] = c;
            }
        }
        return;
    }

    /* ---------- HCI events ----------------------------- */
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }
    if (hci_event_packet_get_type(packet)
        != HCI_EVENT_GATTSERVICE_META) {
        return;
    }

    switch (hci_event_gattservice_meta_get_subevent_code(
                packet)) {
    case GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED:
        ble_con_handle =
            gattservice_subevent_spp_service_connected_get_con_handle(
                packet);
        ble_is_connected = true;
        ESP_LOGI(TAG, "BLE console connected (handle 0x%04x)",
                 ble_con_handle);
        {
            const char *welcome =
                "Bobbycar BLE Console\r\n> ";
            tx_put(welcome, (int)strlen(welcome));
            nordic_spp_service_server_request_can_send_now(
                &send_request, ble_con_handle);
        }
        break;

    case GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED:
        ESP_LOGI(TAG, "BLE console disconnected");
        ble_con_handle = HCI_CON_HANDLE_INVALID;
        ble_is_connected = false;
        rx_pos = 0;
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------ */
/*  Public API                                            */
/* ------------------------------------------------------ */

/* See ble_console.h for doc comment. */
void ble_console_init(void)
{
    tx_mux = xSemaphoreCreateMutex();
    nordic_spp_service_server_init(&ble_nus_handler);
    send_request.callback = &ble_can_send_cb;
    ESP_LOGI(TAG, "BLE NUS console initialized");
}

/* See ble_console.h for doc comment. */
bool ble_console_connected(void)
{
    return ble_is_connected;
}

/* See ble_console.h for doc comment. */
void ble_console_send(const char *data, int len)
{
    if (!ble_is_connected
        || ble_con_handle == HCI_CON_HANDLE_INVALID) {
        return;
    }
    tx_put(data, len);
    nordic_spp_service_server_request_can_send_now(
        &send_request, ble_con_handle);
}

/* See ble_console.h for doc comment. */
int console_printf(const char *fmt, ...)
{
    va_list ap;
    char buf[256];

    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    int len = n;
    if (len < 0) {
        len = 0;
    }
    if (len >= (int)sizeof(buf)) {
        len = (int)sizeof(buf) - 1;
    }

    /* Always write to UART (stdout). */
    fwrite(buf, 1, (size_t)len, stdout);
    fflush(stdout);

    /* Mirror to BLE if a client is connected. */
    if (ble_is_connected && len > 0) {
        ble_console_send(buf, len);
    }

    return n;
}
