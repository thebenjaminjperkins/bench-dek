#ifndef HOST_TRANSPORT_ADAPTER_H
#define HOST_TRANSPORT_ADAPTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dek_protocol/dek_transport.h"
#include "message-types/dek_capabilities.h"
#include "message-types/dek_descriptor.h"

#define HOST_TRANSPORT_ADAPTER_PACKET_MAX 512u
#define HOST_TRANSPORT_ADAPTER_CAPABILITY_MAX 128u
#define HOST_TRANSPORT_ADAPTER_COMMAND_MAX 128u

typedef bool (*host_transport_exchange_fn)(
    void *ctx,
    const uint8_t *tx_buffer,
    uint16_t tx_length,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length);

typedef struct
{
    uint8_t selected_protocol_version;
    uint8_t module_flags;
} host_transport_hello_result_t;

typedef struct
{
    uint16_t channel_id;
    uint16_t service_flags;
    uint16_t accepted_config_length;
} host_transport_open_result_t;

typedef struct
{
    dek_transport_t transport;
    void *exchange_ctx;
    host_transport_exchange_fn exchange;
} host_transport_adapter_t;

bool host_transport_adapter_init(
    host_transport_adapter_t *adapter,
    host_transport_exchange_fn exchange,
    void *exchange_ctx);

bool host_transport_adapter_send_hello(
    host_transport_adapter_t *adapter,
    host_transport_hello_result_t *out_result);

bool host_transport_adapter_get_descriptor(
    host_transport_adapter_t *adapter,
    dek_descriptor_payload_t *out_descriptor);

bool host_transport_adapter_get_capabilities(
    host_transport_adapter_t *adapter,
    uint16_t manifest_offset,
    uint16_t requested_length,
    uint8_t *out_manifest_chunk,
    uint16_t manifest_chunk_capacity,
    uint16_t *out_manifest_total_bytes,
    uint16_t *out_chunk_offset,
    uint16_t *out_chunk_length);

bool host_transport_adapter_open_capability(
    host_transport_adapter_t *adapter,
    const char *capability_id,
    uint16_t capability_version,
    const void *config,
    uint16_t config_length,
    host_transport_open_result_t *out_result);

bool host_transport_adapter_close_capability(
    host_transport_adapter_t *adapter,
    uint16_t channel_id);

bool host_transport_adapter_send_command(
    host_transport_adapter_t *adapter,
    uint16_t channel_id,
    uint8_t command_id,
    const void *command_payload,
    uint16_t command_payload_length,
    uint8_t *out_status,
    void *out_response_payload,
    uint16_t response_payload_capacity,
    uint16_t *out_response_payload_length);

#endif
