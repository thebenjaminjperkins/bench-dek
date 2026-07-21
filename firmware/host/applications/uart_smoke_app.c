#include "uart_smoke_app.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "esp_check.h"

#include "dek_message.h"
#include "dek_packet.h"
#include "dek_transport.h"
#include "message-types/dek_hello.h"

#define DEK_UART_PORT UART_NUM_0
#define DEK_UART_TX_PIN 43
#define DEK_UART_RX_PIN 44
#define DEK_UART_BAUD 115200

#define DEK_UART_RX_BUFFER_SIZE 1024
#define DEK_UART_TX_BUFFER_SIZE 1024
#define DEK_PACKET_BUFFER_SIZE 256
#define DEK_HELLO_RETRY_MS 2000
#define DEK_COMMAND_DELAY_MS 500
#define DEK_STATUS_INTERVAL_MS 2000

typedef struct
{
    uint8_t selected_protocol_version;
    uint8_t module_flags;
    uint16_t reserved;
} dek_hello_ack_payload_t;

typedef enum
{
    DEK_RX_STATUS_SYNCING = 0,
    DEK_RX_STATUS_IN_PROGRESS,
    DEK_RX_STATUS_PACKET_READY,
    DEK_RX_STATUS_INVALID_PACKET,
    DEK_RX_STATUS_BUFFER_OVERFLOW
} dek_rx_status_t;

typedef struct
{
    uint8_t buffer[DEK_PACKET_BUFFER_SIZE];
    uint16_t length;
    uint16_t expected_length;
} dek_packet_stream_t;

typedef struct
{
    dek_transport_t transport;
    dek_packet_stream_t rx_stream;

    bool hello_ack_received;
    bool command_sent;
    bool response_received;

    uint32_t packets_received;
    uint32_t invalid_packets;
    uint32_t buffer_overflows;
    uint32_t bytes_received;
    uint32_t bytes_sent;

    TickType_t last_hello_tick;
    TickType_t hello_ack_tick;
    TickType_t last_status_tick;
} dek_host_uart_state_t;

static void dek_packet_stream_reset(dek_packet_stream_t *stream)
{
    if (stream == NULL)
    {
        return;
    }

    stream->length = 0u;
    stream->expected_length = 0u;
}

static void uart_link_init(void)
{
    const uart_config_t config = {
        .baud_rate = DEK_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(
        DEK_UART_PORT,
        DEK_UART_RX_BUFFER_SIZE,
        DEK_UART_TX_BUFFER_SIZE,
        0,
        NULL,
        0));

    ESP_ERROR_CHECK(uart_param_config(DEK_UART_PORT, &config));
    ESP_ERROR_CHECK(uart_set_pin(
        DEK_UART_PORT,
        DEK_UART_TX_PIN,
        DEK_UART_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_flush_input(DEK_UART_PORT));
}

static bool uart_send_packet(
    dek_host_uart_state_t *state,
    const uint8_t *buffer,
    uint16_t length,
    const char *label)
{
    int written;

    if (state == NULL || buffer == NULL || length == 0u)
    {
        return false;
    }

    written = uart_write_bytes(
        DEK_UART_PORT,
        (const char *)buffer,
        length);

    if (written != (int)length)
    {
        printf("%s write short: wanted=%u got=%d\n", label, length, written);
        return false;
    }

    state->bytes_sent += (uint32_t)written;

    printf("%s TX (%u bytes):", label, length);

    for (uint16_t i = 0; i < length; ++i)
    {
        printf(" %02X", buffer[i]);
    }

    printf("\n");
    return true;
}

static bool send_hello_packet(dek_host_uart_state_t *state)
{
    uint8_t tx_buffer[DEK_PACKET_BUFFER_SIZE];
    uint16_t encoded_length = 0u;

    if (!dek_transport_send_hello(
            &state->transport,
            tx_buffer,
            sizeof(tx_buffer),
            &encoded_length))
    {
        printf("Failed to build HELLO packet\n");
        return false;
    }

    state->last_hello_tick = xTaskGetTickCount();
    return uart_send_packet(state, tx_buffer, encoded_length, "HELLO");
}

static bool send_command_packet(
    dek_host_uart_state_t *state,
    const char *message)
{
    uint8_t tx_buffer[DEK_PACKET_BUFFER_SIZE];
    uint16_t encoded_length = 0u;
    uint16_t payload_length;

    if (state == NULL || message == NULL)
    {
        return false;
    }

    payload_length = (uint16_t)strlen(message);

    if (!dek_transport_send(
            &state->transport,
            DEK_MSG_COMMAND,
            0u,
            (const uint8_t *)message,
            payload_length,
            tx_buffer,
            sizeof(tx_buffer),
            &encoded_length))
    {
        printf("Failed to build COMMAND packet\n");
        return false;
    }

    state->command_sent = uart_send_packet(state, tx_buffer, encoded_length, "COMMAND");
    return state->command_sent;
}

static bool decode_hello_ack(
    dek_hello_ack_payload_t *hello_ack,
    const dek_packet_t *packet)
{
    if (hello_ack == NULL || packet == NULL || packet->payload == NULL)
    {
        return false;
    }

    if (packet->header.message_type != DEK_MSG_HELLO_ACK ||
        packet->header.payload_length != sizeof(dek_hello_payload_t))
    {
        return false;
    }

    hello_ack->selected_protocol_version = packet->payload[0];
    hello_ack->module_flags = packet->payload[1];
    hello_ack->reserved =
        (uint16_t)packet->payload[2] |
        ((uint16_t)packet->payload[3] << 8);

    return true;
}

static void print_ascii_payload(
    const uint8_t *payload,
    uint16_t payload_length)
{
    printf("\"");

    for (uint16_t i = 0; i < payload_length; ++i)
    {
        uint8_t byte = payload[i];

        if (byte >= 32u && byte <= 126u)
        {
            printf("%c", (char)byte);
        }
        else
        {
            printf(".");
        }
    }

    printf("\"");
}

static void handle_packet(
    dek_host_uart_state_t *state,
    const dek_packet_t *packet)
{
    dek_hello_ack_payload_t hello_ack;

    state->packets_received++;

    printf(
        "RX packet: type=%u seq=%u channel=%u payload=%u\n",
        packet->header.message_type,
        packet->header.sequence_number,
        packet->header.channel_id,
        packet->header.payload_length);

    switch ((dek_message_type_t)packet->header.message_type)
    {
        case DEK_MSG_HELLO_ACK:
            if (decode_hello_ack(&hello_ack, packet))
            {
                state->hello_ack_received = true;
                state->hello_ack_tick = xTaskGetTickCount();
                printf(
                    "HELLO_ACK received: version=%u flags=0x%02X\n",
                    hello_ack.selected_protocol_version,
                    hello_ack.module_flags);
            }
            else
            {
                printf("HELLO_ACK decode failed\n");
            }
            break;

        case DEK_MSG_RESPONSE:
            state->response_received = true;
            printf("RESPONSE payload ");
            print_ascii_payload(packet->payload, packet->header.payload_length);
            printf("\n");
            break;

        default:
            printf("Unhandled packet type: %u\n", packet->header.message_type);
            break;
    }
}

static dek_rx_status_t feed_rx_byte(
    dek_packet_stream_t *stream,
    uint8_t byte,
    dek_packet_t *packet)
{
    dek_packet_header_t header;

    if (stream == NULL || packet == NULL)
    {
        return DEK_RX_STATUS_INVALID_PACKET;
    }

    if (stream->length == 0u)
    {
        if (byte != DEK_PACKET_MAGIC_BYTE0)
        {
            return DEK_RX_STATUS_SYNCING;
        }
    }
    else if (stream->length == 1u)
    {
        if (byte != DEK_PACKET_MAGIC_BYTE1)
        {
            stream->buffer[0] = byte;
            stream->length = (byte == DEK_PACKET_MAGIC_BYTE0) ? 1u : 0u;
            return DEK_RX_STATUS_SYNCING;
        }
    }

    if (stream->length >= DEK_PACKET_BUFFER_SIZE)
    {
        dek_packet_stream_reset(stream);
        return DEK_RX_STATUS_BUFFER_OVERFLOW;
    }

    stream->buffer[stream->length++] = byte;

    if (stream->length < DEK_PACKET_HEADER_SIZE)
    {
        return DEK_RX_STATUS_IN_PROGRESS;
    }

    if (stream->expected_length == 0u)
    {
        if (!dek_packet_decode_header(&header, stream->buffer, stream->length))
        {
            dek_packet_stream_reset(stream);
            return DEK_RX_STATUS_INVALID_PACKET;
        }

        stream->expected_length = (uint16_t)(DEK_PACKET_OVERHEAD + header.payload_length);

        if (stream->expected_length > DEK_PACKET_BUFFER_SIZE)
        {
            dek_packet_stream_reset(stream);
            return DEK_RX_STATUS_BUFFER_OVERFLOW;
        }
    }

    if (stream->length < stream->expected_length)
    {
        return DEK_RX_STATUS_IN_PROGRESS;
    }

    if (!dek_packet_decode(packet, stream->buffer, stream->expected_length))
    {
        dek_packet_stream_reset(stream);
        return DEK_RX_STATUS_INVALID_PACKET;
    }

    dek_packet_stream_reset(stream);
    return DEK_RX_STATUS_PACKET_READY;
}

static void process_uart_rx(dek_host_uart_state_t *state)
{
    uint8_t rx_buffer[64];
    int bytes_read;

    bytes_read = uart_read_bytes(
        DEK_UART_PORT,
        rx_buffer,
        sizeof(rx_buffer),
        pdMS_TO_TICKS(20));

    if (bytes_read <= 0)
    {
        return;
    }

    state->bytes_received += (uint32_t)bytes_read;

    for (int i = 0; i < bytes_read; ++i)
    {
        dek_packet_t packet = { 0 };
        dek_rx_status_t status = feed_rx_byte(&state->rx_stream, rx_buffer[i], &packet);

        if (status == DEK_RX_STATUS_PACKET_READY)
        {
            handle_packet(state, &packet);
        }
        else if (status == DEK_RX_STATUS_INVALID_PACKET)
        {
            state->invalid_packets++;
            printf("Host UART receiver rejected malformed packet bytes\n");
        }
        else if (status == DEK_RX_STATUS_BUFFER_OVERFLOW)
        {
            state->buffer_overflows++;
            printf("Host UART receiver buffer overflow\n");
        }
    }
}

static void maybe_send_follow_up(dek_host_uart_state_t *state)
{
    static const char k_message[] = "UART bring-up message from ESP host";
    TickType_t now = xTaskGetTickCount();

    if (!state->hello_ack_received)
    {
        if ((now - state->last_hello_tick) >= pdMS_TO_TICKS(DEK_HELLO_RETRY_MS))
        {
            (void)send_hello_packet(state);
        }

        return;
    }

    if (!state->command_sent &&
        (now - state->hello_ack_tick) >= pdMS_TO_TICKS(DEK_COMMAND_DELAY_MS))
    {
        (void)send_command_packet(state, k_message);
    }
}

static void maybe_report_status(dek_host_uart_state_t *state)
{
    TickType_t now = xTaskGetTickCount();

    if ((now - state->last_status_tick) < pdMS_TO_TICKS(DEK_STATUS_INTERVAL_MS))
    {
        return;
    }

    state->last_status_tick = now;

    printf(
        "[host-uart] bytes_tx=%lu bytes_rx=%lu packets_rx=%lu invalid=%lu overflow=%lu "
        "hello_ack=%u command_sent=%u response=%u\n",
        (unsigned long)state->bytes_sent,
        (unsigned long)state->bytes_received,
        (unsigned long)state->packets_received,
        (unsigned long)state->invalid_packets,
        (unsigned long)state->buffer_overflows,
        state->hello_ack_received ? 1u : 0u,
        state->command_sent ? 1u : 0u,
        state->response_received ? 1u : 0u);
}

void uart_smoke_app_run(void)
{
    dek_host_uart_state_t state;

    memset(&state, 0, sizeof(state));
    dek_transport_init(&state.transport);
    dek_packet_stream_reset(&state.rx_stream);

    uart_link_init();

    printf(
        "UART smoke test starting on Tuesday, July 21, 2026 using UART%u TX=%d RX=%d baud=%d\n",
        (unsigned int)DEK_UART_PORT,
        DEK_UART_TX_PIN,
        DEK_UART_RX_PIN,
        DEK_UART_BAUD);

    (void)send_hello_packet(&state);

    while (1)
    {
        process_uart_rx(&state);
        maybe_send_follow_up(&state);
        maybe_report_status(&state);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
