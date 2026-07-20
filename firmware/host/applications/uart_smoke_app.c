#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"

#include "uart_smoke_app.h"
#include "dek_transport.h"

#define DEK_UART_PORT      UART_NUM_0
#define DEK_UART_TX_PIN    43
#define DEK_UART_RX_PIN    44
#define DEK_UART_BAUD      115200

void uart_smoke_app_run(void)
{
    const uart_config_t config = {
        .baud_rate = DEK_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(
        DEK_UART_PORT,
        1024,
        0,
        0,
        NULL,
        0);

    uart_param_config(
        DEK_UART_PORT,
        &config);

    uart_set_pin(
        DEK_UART_PORT,
        DEK_UART_TX_PIN,
        DEK_UART_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE);

    dek_transport_t transport;
    dek_transport_init(&transport);

    uint8_t tx_buffer[256];
    uint16_t encoded_length;


    while (1)
    {
        if (dek_transport_send_hello(
            &transport,
            tx_buffer,
            sizeof(tx_buffer),
            &encoded_length))
        {
            uart_write_bytes(
                DEK_UART_PORT,
                tx_buffer,
                encoded_length);
        }
    }
}