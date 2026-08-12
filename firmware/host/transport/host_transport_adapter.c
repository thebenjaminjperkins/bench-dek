#include "transport/host_transport_adapter.h"

#include <limits.h>
#include <string.h>

#include "dek_message.h"
#include "dek_protocol/message-types/dek_hello.h"
#include "message-types/dek_open_capability.h"
#include "message-types/dek_service_command.h"

typedef struct
{
    uint8_t selected_protocol_version;
    uint8_t module_flags;
    uint16_t reserved;
} host_transport_hello_ack_payload_t;

static bool host_transport_adapter_exchange_raw(
    host_transport_adapter_t *adapter,
    dek_message_type_t message_type,
    uint16_t channel_id,
    const void *payload,
    uint16_t payload_length,
    dek_message_type_t expected_response_type,
    uint16_t expected_response_channel,
    uint8_t *out_response_payload,
    uint16_t response_payload_capacity,
    uint16_t *out_response_payload_length)
{
    uint8_t tx_buffer[HOST_TRANSPORT_ADAPTER_PACKET_MAX];
    uint8_t rx_buffer[HOST_TRANSPORT_ADAPTER_PACKET_MAX];
    uint16_t tx_length = 0u;
    uint16_t rx_length = 0u;
    dek_packet_t response_packet;

    if (adapter == NULL ||
        adapter->exchange == NULL ||
        (payload == NULL && payload_length > 0u))
    {
        return false;
    }

    if (!dek_transport_send(
            &adapter->transport,
            message_type,
            channel_id,
            (const uint8_t *)payload,
            payload_length,
            tx_buffer,
            sizeof(tx_buffer),
            &tx_length))
    {
        return false;
    }

    if (!adapter->exchange(
            adapter->exchange_ctx,
            tx_buffer,
            tx_length,
            rx_buffer,
            sizeof(rx_buffer),
            &rx_length))
    {
        return false;
    }

    if (!dek_transport_receive(&adapter->transport, &response_packet, rx_buffer, rx_length))
    {
        return false;
    }

    if (response_packet.header.message_type != expected_response_type)
    {
        return false;
    }

    if (expected_response_channel != UINT16_MAX &&
        response_packet.header.channel_id != expected_response_channel)
    {
        return false;
    }

    if (out_response_payload_length != NULL)
    {
        *out_response_payload_length = response_packet.header.payload_length;
    }

    if (response_packet.header.payload_length == 0u)
    {
        return true;
    }

    if (out_response_payload == NULL ||
        response_payload_capacity < response_packet.header.payload_length ||
        response_packet.payload == NULL)
    {
        return false;
    }

    memcpy(
        out_response_payload,
        response_packet.payload,
        response_packet.header.payload_length);
    return true;
}

bool host_transport_adapter_init(
    host_transport_adapter_t *adapter,
    host_transport_exchange_fn exchange,
    void *exchange_ctx)
{
    if (adapter == NULL || exchange == NULL)
    {
        return false;
    }

    memset(adapter, 0, sizeof(*adapter));
    dek_transport_init(&adapter->transport);
    adapter->exchange = exchange;
    adapter->exchange_ctx = exchange_ctx;
    return true;
}

bool host_transport_adapter_send_hello(
    host_transport_adapter_t *adapter,
    host_transport_hello_result_t *out_result)
{
    uint8_t tx_buffer[HOST_TRANSPORT_ADAPTER_PACKET_MAX];
    uint8_t payload_buffer[sizeof(host_transport_hello_ack_payload_t)];
    uint8_t rx_buffer[HOST_TRANSPORT_ADAPTER_PACKET_MAX];
    uint16_t tx_length = 0u;
    uint16_t rx_length = 0u;
    dek_packet_t response_packet;
    host_transport_hello_ack_payload_t hello_ack;

    if (adapter == NULL || adapter->exchange == NULL || out_result == NULL)
    {
        return false;
    }

    if (!dek_transport_send_hello(
            &adapter->transport,
            tx_buffer,
            sizeof(tx_buffer),
            &tx_length))
    {
        return false;
    }

    if (!adapter->exchange(
            adapter->exchange_ctx,
            tx_buffer,
            tx_length,
            rx_buffer,
            sizeof(rx_buffer),
            &rx_length))
    {
        return false;
    }

    if (!dek_transport_receive(&adapter->transport, &response_packet, rx_buffer, rx_length) ||
        response_packet.header.message_type != DEK_MSG_HELLO_ACK ||
        response_packet.header.channel_id != 0u ||
        response_packet.header.payload_length != sizeof(hello_ack) ||
        response_packet.payload == NULL)
    {
        return false;
    }

    memcpy(payload_buffer, response_packet.payload, sizeof(payload_buffer));
    memcpy(&hello_ack, payload_buffer, sizeof(hello_ack));
    out_result->selected_protocol_version = hello_ack.selected_protocol_version;
    out_result->module_flags = hello_ack.module_flags;
    return true;
}

bool host_transport_adapter_get_descriptor(
    host_transport_adapter_t *adapter,
    dek_descriptor_payload_t *out_descriptor)
{
    uint16_t payload_length = 0u;

    if (out_descriptor == NULL)
    {
        return false;
    }

    if (!host_transport_adapter_exchange_raw(
            adapter,
            DEK_MSG_GET_DESCRIPTOR,
            0u,
            NULL,
            0u,
            DEK_MSG_DESCRIPTOR,
            0u,
            (uint8_t *)out_descriptor,
            sizeof(*out_descriptor),
            &payload_length))
    {
        return false;
    }

    return payload_length == sizeof(*out_descriptor) &&
           dek_descriptor_payload_is_valid(out_descriptor);
}

bool host_transport_adapter_get_capabilities(
    host_transport_adapter_t *adapter,
    uint16_t manifest_offset,
    uint16_t requested_length,
    uint8_t *out_manifest_chunk,
    uint16_t manifest_chunk_capacity,
    uint16_t *out_manifest_total_bytes,
    uint16_t *out_chunk_offset,
    uint16_t *out_chunk_length)
{
    dek_get_capabilities_payload_t request;
    uint8_t response_buffer[HOST_TRANSPORT_ADAPTER_CAPABILITY_MAX];
    dek_capabilities_payload_header_t header;
    uint16_t response_length = 0u;
    uint16_t chunk_length;

    if (out_manifest_total_bytes == NULL ||
        out_chunk_offset == NULL ||
        out_chunk_length == NULL)
    {
        return false;
    }

    dek_get_capabilities_payload_init(&request);
    request.manifest_offset = manifest_offset;
    request.requested_length = requested_length;

    if (!host_transport_adapter_exchange_raw(
            adapter,
            DEK_MSG_GET_CAPABILITIES,
            0u,
            &request,
            sizeof(request),
            DEK_MSG_CAPABILITIES,
            0u,
            response_buffer,
            sizeof(response_buffer),
            &response_length))
    {
        return false;
    }

    if (response_length < sizeof(header))
    {
        return false;
    }

    memcpy(&header, response_buffer, sizeof(header));
    chunk_length = header.chunk_length;
    if ((uint16_t)(sizeof(header) + chunk_length) != response_length ||
        chunk_length > manifest_chunk_capacity ||
        (chunk_length > 0u && out_manifest_chunk == NULL))
    {
        return false;
    }

    if (chunk_length > 0u)
    {
        memcpy(out_manifest_chunk, response_buffer + sizeof(header), chunk_length);
    }

    *out_manifest_total_bytes = header.manifest_total_bytes;
    *out_chunk_offset = header.chunk_offset;
    *out_chunk_length = header.chunk_length;
    return true;
}

bool host_transport_adapter_open_capability(
    host_transport_adapter_t *adapter,
    const char *capability_id,
    uint16_t capability_version,
    const void *config,
    uint16_t config_length,
    host_transport_open_result_t *out_result)
{
    uint8_t request_buffer[HOST_TRANSPORT_ADAPTER_CAPABILITY_MAX];
    uint8_t response_buffer[sizeof(dek_open_ack_payload_header_t)];
    dek_open_capability_payload_header_t request_header;
    dek_open_ack_payload_header_t response_header;
    uint16_t response_length = 0u;
    size_t capability_id_length;
    uint16_t request_length;

    if (adapter == NULL ||
        out_result == NULL ||
        !dek_capability_id_is_valid(capability_id) ||
        (config == NULL && config_length > 0u))
    {
        return false;
    }

    capability_id_length = strlen(capability_id);
    request_length = (uint16_t)(
        sizeof(request_header) +
        capability_id_length +
        config_length);
    if (request_length > sizeof(request_buffer))
    {
        return false;
    }

    dek_open_capability_payload_header_init(&request_header);
    request_header.capability_id_length = (uint8_t)capability_id_length;
    request_header.capability_version = capability_version;
    request_header.open_flags = DEK_OPEN_CAPABILITY_FLAG_NONE;
    request_header.config_length = config_length;

    memcpy(request_buffer, &request_header, sizeof(request_header));
    memcpy(request_buffer + sizeof(request_header), capability_id, capability_id_length);
    if (config_length > 0u)
    {
        memcpy(
            request_buffer + sizeof(request_header) + capability_id_length,
            config,
            config_length);
    }

    if (!host_transport_adapter_exchange_raw(
            adapter,
            DEK_MSG_OPEN_CAPABILITY,
            0u,
            request_buffer,
            request_length,
            DEK_MSG_OPEN_ACK,
            0u,
            response_buffer,
            sizeof(response_buffer),
            &response_length))
    {
        return false;
    }

    if (response_length != sizeof(response_header))
    {
        return false;
    }

    memcpy(&response_header, response_buffer, sizeof(response_header));
    if (!dek_open_ack_payload_header_is_valid(&response_header))
    {
        return false;
    }

    out_result->channel_id = response_header.channel_id;
    out_result->service_flags = response_header.service_flags;
    out_result->accepted_config_length = response_header.accepted_config_length;
    return true;
}

bool host_transport_adapter_close_capability(
    host_transport_adapter_t *adapter,
    uint16_t channel_id)
{
    dek_close_capability_payload_t request;
    uint16_t response_length = 0u;

    dek_close_capability_payload_init(&request);
    request.target_channel_id = channel_id;

    return host_transport_adapter_exchange_raw(
        adapter,
        DEK_MSG_CLOSE_CAPABILITY,
        0u,
        &request,
        sizeof(request),
        DEK_MSG_CLOSE_ACK,
        0u,
        NULL,
        0u,
        &response_length) &&
        response_length == 0u;
}

bool host_transport_adapter_send_command(
    host_transport_adapter_t *adapter,
    uint16_t channel_id,
    uint8_t command_id,
    const void *command_payload,
    uint16_t command_payload_length,
    uint8_t *out_status,
    void *out_response_payload,
    uint16_t response_payload_capacity,
    uint16_t *out_response_payload_length)
{
    uint8_t request_buffer[HOST_TRANSPORT_ADAPTER_COMMAND_MAX];
    uint8_t response_buffer[HOST_TRANSPORT_ADAPTER_COMMAND_MAX];
    dek_service_command_request_header_t request_header;
    dek_service_command_response_header_t response_header;
    uint16_t request_length;
    uint16_t response_length = 0u;
    uint16_t body_length;

    if (adapter == NULL ||
        channel_id == 0u ||
        command_id == 0u ||
        (command_payload == NULL && command_payload_length > 0u))
    {
        return false;
    }

    request_length = (uint16_t)(sizeof(request_header) + command_payload_length);
    if (request_length > sizeof(request_buffer))
    {
        return false;
    }

    dek_service_command_request_header_init(&request_header);
    request_header.command_id = command_id;
    memcpy(request_buffer, &request_header, sizeof(request_header));
    if (command_payload_length > 0u)
    {
        memcpy(
            request_buffer + sizeof(request_header),
            command_payload,
            command_payload_length);
    }

    if (!host_transport_adapter_exchange_raw(
            adapter,
            DEK_MSG_COMMAND,
            channel_id,
            request_buffer,
            request_length,
            DEK_MSG_RESPONSE,
            channel_id,
            response_buffer,
            sizeof(response_buffer),
            &response_length))
    {
        return false;
    }

    if (response_length < sizeof(response_header))
    {
        return false;
    }

    memcpy(&response_header, response_buffer, sizeof(response_header));
    if (!dek_service_command_response_header_is_valid(&response_header) ||
        response_header.command_id != command_id)
    {
        return false;
    }

    body_length = (uint16_t)(response_length - sizeof(response_header));
    if (out_status != NULL)
    {
        *out_status = response_header.status;
    }
    else if (response_header.status != (uint8_t)DEK_SERVICE_COMMAND_STATUS_OK)
    {
        return false;
    }

    if (out_response_payload_length != NULL)
    {
        *out_response_payload_length = body_length;
    }

    if (body_length == 0u)
    {
        return true;
    }

    if (out_response_payload == NULL || response_payload_capacity < body_length)
    {
        return false;
    }

    memcpy(
        out_response_payload,
        response_buffer + sizeof(response_header),
        body_length);
    return true;
}
