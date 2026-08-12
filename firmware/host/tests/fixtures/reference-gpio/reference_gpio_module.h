#ifndef REFERENCE_GPIO_MODULE_H
#define REFERENCE_GPIO_MODULE_H

#include <stdbool.h>
#include <stdint.h>

#include "dek_protocol/dek_transport.h"
#include "message-types/dek_descriptor.h"

#define REFERENCE_GPIO_MODULE_PIN_COUNT 16u
#define REFERENCE_GPIO_MODULE_MAX_CHANNELS 4u
#define REFERENCE_GPIO_MODULE_MANIFEST_MAX 64u

typedef struct
{
    bool occupied;
    uint16_t channel_id;
} reference_gpio_module_channel_t;

typedef struct
{
    dek_transport_t transport;
    dek_descriptor_payload_t descriptor;
    uint8_t capability_manifest[REFERENCE_GPIO_MODULE_MANIFEST_MAX];
    uint16_t capability_manifest_length;
    uint16_t next_channel_id;
    reference_gpio_module_channel_t channels[REFERENCE_GPIO_MODULE_MAX_CHANNELS];
    uint8_t pin_modes[REFERENCE_GPIO_MODULE_PIN_COUNT];
    uint8_t pin_values[REFERENCE_GPIO_MODULE_PIN_COUNT];
} reference_gpio_module_t;

bool reference_gpio_module_init(reference_gpio_module_t *module);
bool reference_gpio_module_exchange(
    void *ctx,
    const uint8_t *tx_buffer,
    uint16_t tx_length,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length);

#endif
