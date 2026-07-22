#include "spi_smoke_app.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/spi_master.h"
#include "esp_attr.h"

#include "dek_protocol/dek_transport.h"
#include "dek_protocol/dek_packet.h"
#include "dek_protocol/message-types/dek_hello.h"

#define PIN_NUM_CS   10
#define PIN_NUM_MOSI 11
#define PIN_NUM_CLK  12
#define PIN_NUM_MISO 13

#define SPI_CLOCK_HZ 1000000
#define SPI_FRAME_SIZE 64

static spi_device_handle_t s_spi;
static DMA_ATTR uint8_t s_tx_buffer[SPI_FRAME_SIZE];
static DMA_ATTR uint8_t s_rx_buffer[SPI_FRAME_SIZE];

typedef struct
{
    uint8_t selected_protocol_version;
    uint8_t module_flags;
    uint16_t reserved;
} dek_hello_ack_payload_t;

static esp_err_t spi_init(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 256,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 4,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(
        SPI2_HOST,
        &buscfg,
        SPI_DMA_CH_AUTO));

    ESP_ERROR_CHECK(spi_bus_add_device(
        SPI2_HOST,
        &devcfg,
        &s_spi));

    return ESP_OK;
}

static esp_err_t spi_send_recv(
    const uint8_t *tx,
    uint8_t *rx,
    size_t len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    return spi_device_transmit(s_spi, &t);
}

static bool build_poll_frame(
    dek_transport_t *transport,
    uint8_t *buffer,
    uint16_t buffer_size,
    uint16_t *encoded_length)
{
    return dek_transport_send(
        transport,
        DEK_MSG_POLL,
        0,
        NULL,
        0,
        buffer,
        buffer_size,
        encoded_length);
}

static bool decode_hello_ack(
    dek_hello_ack_payload_t *hello_ack,
    const dek_packet_t *packet)
{
    if (hello_ack == NULL || packet == NULL)
    {
        return false;
    }

    if (packet->header.message_type != DEK_MSG_HELLO_ACK ||
        packet->header.payload_length != sizeof(*hello_ack) ||
        packet->payload == NULL)
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

void spi_smoke_app_run(void)
{
    dek_transport_t transport;
    dek_packet_t packet;
    dek_hello_ack_payload_t hello_ack;

    uint16_t hello_length = 0;
    uint16_t poll_length = 0;

    dek_transport_init(&transport);

    ESP_ERROR_CHECK(spi_init());

    printf("SPI smoke test starting...\n");

    while (1)
    {
        /*----------------------------------------------------------
         * Send HELLO
         *----------------------------------------------------------*/

        if (!dek_transport_send_hello(
                &transport,
                s_tx_buffer,
                sizeof(s_tx_buffer),
                &hello_length))
        {
            printf("Failed to build HELLO packet\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        printf("encoded_length = %u\n", hello_length);

        memset(s_tx_buffer + hello_length, 0, sizeof(s_tx_buffer) - hello_length);
        memset(s_rx_buffer, 0, sizeof(s_rx_buffer));

        printf("TX (%u bytes): ", hello_length);

        for (int i = 0; i < hello_length; i++)
        {
            printf("%02X ", s_tx_buffer[i]);
        }

        printf("\n");


        ESP_ERROR_CHECK(
            spi_send_recv(
                s_tx_buffer,
                s_rx_buffer,
                sizeof(s_tx_buffer)));

        /*----------------------------------------------------------
         * Clock out the module's queued HELLO_ACK
         *----------------------------------------------------------*/

        if (!build_poll_frame(
                &transport,
                s_tx_buffer,
                sizeof(s_tx_buffer),
                &poll_length))
        {
            printf("Failed to build POLL packet\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        memset(s_tx_buffer + poll_length, 0, sizeof(s_tx_buffer) - poll_length);
        memset(s_rx_buffer, 0, sizeof(s_rx_buffer));

        ESP_ERROR_CHECK(
            spi_send_recv(
                s_tx_buffer,
                s_rx_buffer,
                sizeof(s_tx_buffer)));

        /*----------------------------------------------------------
         * Decode returned packet
         *----------------------------------------------------------*/

        memset(&packet, 0, sizeof(packet));

        if (!dek_packet_decode(
                &packet,
                s_rx_buffer,
                sizeof(s_rx_buffer)))
        {
            printf("No valid packet received\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        printf(
            "Received packet: type=%u seq=%u channel=%u payload=%u\n",
            packet.header.message_type,
            packet.header.sequence_number,
            packet.header.channel_id,
            packet.header.payload_length);

        if (packet.header.message_type != DEK_MSG_HELLO_ACK)
        {
            printf("Unexpected packet type\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (!decode_hello_ack(
                &hello_ack,
                &packet))
        {
            printf("HELLO_ACK decode failed\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        printf("HELLO_ACK received!\n");
        printf("Selected protocol version: %u\n",
               hello_ack.selected_protocol_version);
        printf("Flags: 0x%04X\n",
               hello_ack.module_flags);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
