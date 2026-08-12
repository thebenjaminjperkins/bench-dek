#include "tests/fixtures/reference-gpio/reference_gpio_module.h"

#include <string.h>

#include "capabilities/dek_capability_ids.h"
#include "capabilities/dek_gpio_digital.h"
#include "dek_message.h"
#include "dek_protocol/dek_packet.h"
#include "message-types/dek_capabilities.h"
#include "message-types/dek_open_capability.h"
#include "message-types/dek_service_command.h"

typedef struct
{
    uint8_t selected_protocol_version;
    uint8_t module_flags;
    uint16_t reserved;
} reference_gpio_module_hello_ack_payload_t;

#define REFERENCE_GPIO_MODULE_COMMAND_RESPONSE_MAX 128u

static const uint8_t s_reference_gpio_module_id[DEK_MODULE_ID_SIZE] = {
    0x47u, 0x50u, 0x49u, 0x4Fu,
    0x00u, 0x01u, 0x02u, 0x03u,
    0x10u, 0x20u, 0x30u, 0x40u,
    0x50u, 0x60u, 0x70u, 0x80u
};

static bool reference_gpio_module_send_response(
    reference_gpio_module_t *module,
    dek_message_type_t message_type,
    uint16_t channel_id,
    const void *payload,
    uint16_t payload_length,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length)
{
    return dek_transport_send(
        &module->transport,
        message_type,
        channel_id,
        (const uint8_t *)payload,
        payload_length,
        rx_buffer,
        rx_buffer_size,
        out_rx_length);
}

static reference_gpio_module_channel_t *reference_gpio_module_find_channel(
    reference_gpio_module_t *module,
    uint16_t channel_id)
{
    size_t i;

    if (module == NULL || channel_id == 0u)
    {
        return NULL;
    }

    for (i = 0; i < REFERENCE_GPIO_MODULE_MAX_CHANNELS; ++i)
    {
        if (module->channels[i].occupied &&
            module->channels[i].channel_id == channel_id)
        {
            return &module->channels[i];
        }
    }

    return NULL;
}

static reference_gpio_module_channel_t *reference_gpio_module_allocate_channel(
    reference_gpio_module_t *module)
{
    size_t i;

    if (module == NULL)
    {
        return NULL;
    }

    for (i = 0; i < REFERENCE_GPIO_MODULE_MAX_CHANNELS; ++i)
    {
        if (!module->channels[i].occupied)
        {
            module->channels[i].occupied = true;
            module->channels[i].channel_id = module->next_channel_id++;
            if (module->next_channel_id == 0u)
            {
                module->next_channel_id = 1u;
            }

            return &module->channels[i];
        }
    }

    return NULL;
}

static bool reference_gpio_module_release_channel(
    reference_gpio_module_t *module,
    uint16_t channel_id)
{
    reference_gpio_module_channel_t *channel =
        reference_gpio_module_find_channel(module, channel_id);

    if (channel == NULL)
    {
        return false;
    }

    memset(channel, 0, sizeof(*channel));
    return true;
}

static bool reference_gpio_module_pin_is_valid(uint8_t pin)
{
    return pin < REFERENCE_GPIO_MODULE_PIN_COUNT;
}

static bool reference_gpio_module_build_manifest(reference_gpio_module_t *module)
{
    dek_capability_manifest_entry_header_t entry;
    const char *capability_id = DEK_CAPABILITY_ID_GPIO_DIGITAL;
    size_t capability_id_length = strlen(capability_id);
    uint16_t total_length;

    if (module == NULL)
    {
        return false;
    }

    total_length = (uint16_t)(sizeof(entry) + capability_id_length);
    if (total_length > sizeof(module->capability_manifest))
    {
        return false;
    }

    dek_capability_manifest_entry_header_init(&entry);
    entry.capability_id_length = (uint8_t)capability_id_length;
    entry.capability_version = DEK_CAPABILITY_VERSION_GPIO_DIGITAL;
    entry.resource_policy = (uint8_t)DEK_RESOURCE_POLICY_SHARED;
    entry.command_count = 3u;

    memcpy(module->capability_manifest, &entry, sizeof(entry));
    memcpy(
        module->capability_manifest + sizeof(entry),
        capability_id,
        capability_id_length);
    module->capability_manifest_length = total_length;
    return true;
}

static bool reference_gpio_module_handle_hello(
    reference_gpio_module_t *module,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length)
{
    reference_gpio_module_hello_ack_payload_t hello_ack = {
        .selected_protocol_version = DEK_PROTOCOL_VERSION,
        .module_flags = 0u,
        .reserved = 0u
    };

    return reference_gpio_module_send_response(
        module,
        DEK_MSG_HELLO_ACK,
        0u,
        &hello_ack,
        sizeof(hello_ack),
        rx_buffer,
        rx_buffer_size,
        out_rx_length);
}

static bool reference_gpio_module_handle_get_descriptor(
    reference_gpio_module_t *module,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length)
{
    return reference_gpio_module_send_response(
        module,
        DEK_MSG_DESCRIPTOR,
        0u,
        &module->descriptor,
        sizeof(module->descriptor),
        rx_buffer,
        rx_buffer_size,
        out_rx_length);
}

static bool reference_gpio_module_handle_get_capabilities(
    reference_gpio_module_t *module,
    const dek_packet_t *request_packet,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length)
{
    dek_get_capabilities_payload_t request;
    dek_capabilities_payload_header_t header;
    uint8_t payload[sizeof(header) + REFERENCE_GPIO_MODULE_MANIFEST_MAX];
    uint16_t chunk_length;

    if (request_packet == NULL ||
        request_packet->payload == NULL ||
        request_packet->header.payload_length != sizeof(request))
    {
        return false;
    }

    memcpy(&request, request_packet->payload, sizeof(request));
    if (request.manifest_offset > module->capability_manifest_length)
    {
        return false;
    }

    dek_capabilities_payload_header_init(&header);
    header.manifest_total_bytes = module->capability_manifest_length;
    header.chunk_offset = request.manifest_offset;
    chunk_length = (uint16_t)(module->capability_manifest_length - request.manifest_offset);
    if (request.requested_length > 0u && request.requested_length < chunk_length)
    {
        chunk_length = request.requested_length;
    }
    header.chunk_length = chunk_length;

    memcpy(payload, &header, sizeof(header));
    if (chunk_length > 0u)
    {
        memcpy(
            payload + sizeof(header),
            module->capability_manifest + request.manifest_offset,
            chunk_length);
    }

    return reference_gpio_module_send_response(
        module,
        DEK_MSG_CAPABILITIES,
        0u,
        payload,
        (uint16_t)(sizeof(header) + chunk_length),
        rx_buffer,
        rx_buffer_size,
        out_rx_length);
}

static bool reference_gpio_module_handle_open_capability(
    reference_gpio_module_t *module,
    const dek_packet_t *request_packet,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length)
{
    dek_open_capability_payload_header_t request_header;
    dek_open_ack_payload_header_t response_header;
    reference_gpio_module_channel_t *channel;
    char capability_id[DEK_CAPABILITY_ID_MAX_LENGTH + 1u];
    uint16_t expected_length;

    if (request_packet == NULL ||
        request_packet->payload == NULL ||
        request_packet->header.payload_length < sizeof(request_header))
    {
        return false;
    }

    memcpy(&request_header, request_packet->payload, sizeof(request_header));
    expected_length = (uint16_t)(
        sizeof(request_header) +
        request_header.capability_id_length +
        request_header.config_length);
    if (request_packet->header.payload_length != expected_length ||
        request_header.capability_id_length == 0u ||
        request_header.capability_id_length > DEK_CAPABILITY_ID_MAX_LENGTH)
    {
        return false;
    }

    memcpy(
        capability_id,
        request_packet->payload + sizeof(request_header),
        request_header.capability_id_length);
    capability_id[request_header.capability_id_length] = '\0';

    if (request_header.capability_version != DEK_CAPABILITY_VERSION_GPIO_DIGITAL ||
        strcmp(capability_id, DEK_CAPABILITY_ID_GPIO_DIGITAL) != 0)
    {
        return false;
    }

    channel = reference_gpio_module_allocate_channel(module);
    if (channel == NULL)
    {
        return false;
    }

    dek_open_ack_payload_header_init(&response_header);
    response_header.channel_id = channel->channel_id;
    response_header.accepted_config_length = 0u;

    return reference_gpio_module_send_response(
        module,
        DEK_MSG_OPEN_ACK,
        0u,
        &response_header,
        sizeof(response_header),
        rx_buffer,
        rx_buffer_size,
        out_rx_length);
}

static bool reference_gpio_module_handle_close_capability(
    reference_gpio_module_t *module,
    const dek_packet_t *request_packet,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length)
{
    dek_close_capability_payload_t request;

    if (request_packet == NULL ||
        request_packet->payload == NULL ||
        request_packet->header.payload_length != sizeof(request))
    {
        return false;
    }

    memcpy(&request, request_packet->payload, sizeof(request));
    if (!reference_gpio_module_release_channel(module, request.target_channel_id))
    {
        return false;
    }

    return reference_gpio_module_send_response(
        module,
        DEK_MSG_CLOSE_ACK,
        0u,
        NULL,
        0u,
        rx_buffer,
        rx_buffer_size,
        out_rx_length);
}

static bool reference_gpio_module_send_gpio_response(
    reference_gpio_module_t *module,
    uint16_t channel_id,
    uint8_t command_id,
    uint8_t status,
    const void *body,
    uint16_t body_length,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length)
{
    uint8_t payload[REFERENCE_GPIO_MODULE_COMMAND_RESPONSE_MAX];
    dek_service_command_response_header_t response_header;

    if ((uint16_t)(sizeof(response_header) + body_length) > sizeof(payload))
    {
        return false;
    }

    dek_service_command_response_header_init(&response_header);
    response_header.command_id = command_id;
    response_header.status = status;
    memcpy(payload, &response_header, sizeof(response_header));
    if (body_length > 0u && body != NULL)
    {
        memcpy(payload + sizeof(response_header), body, body_length);
    }

    return reference_gpio_module_send_response(
        module,
        DEK_MSG_RESPONSE,
        channel_id,
        payload,
        (uint16_t)(sizeof(response_header) + body_length),
        rx_buffer,
        rx_buffer_size,
        out_rx_length);
}

static bool reference_gpio_module_handle_gpio_command(
    reference_gpio_module_t *module,
    const dek_packet_t *request_packet,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length)
{
    dek_service_command_request_header_t request_header;
    const uint8_t *body;
    uint16_t body_length;
    uint8_t status = (uint8_t)DEK_SERVICE_COMMAND_STATUS_OK;

    if (module == NULL ||
        request_packet == NULL ||
        request_packet->payload == NULL ||
        request_packet->header.payload_length < sizeof(request_header) ||
        reference_gpio_module_find_channel(module, request_packet->header.channel_id) == NULL)
    {
        return false;
    }

    memcpy(&request_header, request_packet->payload, sizeof(request_header));
    if (!dek_service_command_request_header_is_valid(&request_header))
    {
        return false;
    }

    body = request_packet->payload + sizeof(request_header);
    body_length = (uint16_t)(request_packet->header.payload_length - sizeof(request_header));

    switch ((dek_gpio_digital_command_t)request_header.command_id)
    {
        case DEK_GPIO_DIGITAL_CMD_SET_MODE:
        {
            dek_gpio_digital_set_mode_request_t request;
            dek_gpio_digital_set_mode_response_t response;

            if (body_length != sizeof(request))
            {
                status = (uint8_t)DEK_SERVICE_COMMAND_STATUS_INVALID_ARGUMENT;
                return reference_gpio_module_send_gpio_response(
                    module,
                    request_packet->header.channel_id,
                    request_header.command_id,
                    status,
                    NULL,
                    0u,
                    rx_buffer,
                    rx_buffer_size,
                    out_rx_length);
            }

            memcpy(&request, body, sizeof(request));
            if (!reference_gpio_module_pin_is_valid(request.pin) ||
                !dek_gpio_digital_set_mode_request_is_valid(&request))
            {
                status = (uint8_t)DEK_SERVICE_COMMAND_STATUS_INVALID_ARGUMENT;
                return reference_gpio_module_send_gpio_response(
                    module,
                    request_packet->header.channel_id,
                    request_header.command_id,
                    status,
                    NULL,
                    0u,
                    rx_buffer,
                    rx_buffer_size,
                    out_rx_length);
            }

            module->pin_modes[request.pin] = request.mode;
            memset(&response, 0, sizeof(response));
            response.pin = request.pin;
            response.applied_mode = request.mode;
            return reference_gpio_module_send_gpio_response(
                module,
                request_packet->header.channel_id,
                request_header.command_id,
                status,
                &response,
                sizeof(response),
                rx_buffer,
                rx_buffer_size,
                out_rx_length);
        }

        case DEK_GPIO_DIGITAL_CMD_WRITE:
        {
            dek_gpio_digital_write_request_t request;
            dek_gpio_digital_write_response_t response;

            if (body_length != sizeof(request))
            {
                status = (uint8_t)DEK_SERVICE_COMMAND_STATUS_INVALID_ARGUMENT;
                return reference_gpio_module_send_gpio_response(
                    module,
                    request_packet->header.channel_id,
                    request_header.command_id,
                    status,
                    NULL,
                    0u,
                    rx_buffer,
                    rx_buffer_size,
                    out_rx_length);
            }

            memcpy(&request, body, sizeof(request));
            if (!reference_gpio_module_pin_is_valid(request.pin) ||
                !dek_gpio_digital_write_request_is_valid(&request))
            {
                status = (uint8_t)DEK_SERVICE_COMMAND_STATUS_INVALID_ARGUMENT;
                return reference_gpio_module_send_gpio_response(
                    module,
                    request_packet->header.channel_id,
                    request_header.command_id,
                    status,
                    NULL,
                    0u,
                    rx_buffer,
                    rx_buffer_size,
                    out_rx_length);
            }

            if (module->pin_modes[request.pin] != (uint8_t)DEK_GPIO_DIGITAL_MODE_OUTPUT)
            {
                status = (uint8_t)DEK_SERVICE_COMMAND_STATUS_INVALID_STATE;
                return reference_gpio_module_send_gpio_response(
                    module,
                    request_packet->header.channel_id,
                    request_header.command_id,
                    status,
                    NULL,
                    0u,
                    rx_buffer,
                    rx_buffer_size,
                    out_rx_length);
            }

            module->pin_values[request.pin] = request.value;
            memset(&response, 0, sizeof(response));
            response.pin = request.pin;
            response.applied_value = request.value;
            return reference_gpio_module_send_gpio_response(
                module,
                request_packet->header.channel_id,
                request_header.command_id,
                status,
                &response,
                sizeof(response),
                rx_buffer,
                rx_buffer_size,
                out_rx_length);
        }

        case DEK_GPIO_DIGITAL_CMD_READ:
        {
            dek_gpio_digital_read_request_t request;
            dek_gpio_digital_read_response_t response;

            if (body_length != sizeof(request))
            {
                status = (uint8_t)DEK_SERVICE_COMMAND_STATUS_INVALID_ARGUMENT;
                return reference_gpio_module_send_gpio_response(
                    module,
                    request_packet->header.channel_id,
                    request_header.command_id,
                    status,
                    NULL,
                    0u,
                    rx_buffer,
                    rx_buffer_size,
                    out_rx_length);
            }

            memcpy(&request, body, sizeof(request));
            if (!reference_gpio_module_pin_is_valid(request.pin) ||
                !dek_gpio_digital_read_request_is_valid(&request))
            {
                status = (uint8_t)DEK_SERVICE_COMMAND_STATUS_INVALID_ARGUMENT;
                return reference_gpio_module_send_gpio_response(
                    module,
                    request_packet->header.channel_id,
                    request_header.command_id,
                    status,
                    NULL,
                    0u,
                    rx_buffer,
                    rx_buffer_size,
                    out_rx_length);
            }

            memset(&response, 0, sizeof(response));
            response.pin = request.pin;
            response.value = module->pin_values[request.pin];
            return reference_gpio_module_send_gpio_response(
                module,
                request_packet->header.channel_id,
                request_header.command_id,
                status,
                &response,
                sizeof(response),
                rx_buffer,
                rx_buffer_size,
                out_rx_length);
        }

        default:
            status = (uint8_t)DEK_SERVICE_COMMAND_STATUS_UNSUPPORTED_COMMAND;
            return reference_gpio_module_send_gpio_response(
                module,
                request_packet->header.channel_id,
                request_header.command_id,
                status,
                NULL,
                0u,
                rx_buffer,
                rx_buffer_size,
                out_rx_length);
    }
}

bool reference_gpio_module_init(reference_gpio_module_t *module)
{
    if (module == NULL)
    {
        return false;
    }

    memset(module, 0, sizeof(*module));
    dek_transport_init(&module->transport);
    module->next_channel_id = 1u;

    if (!reference_gpio_module_build_manifest(module))
    {
        return false;
    }

    dek_descriptor_payload_init(&module->descriptor);
    memcpy(
        module->descriptor.module_id,
        s_reference_gpio_module_id,
        sizeof(module->descriptor.module_id));
    module->descriptor.module_family_id = 1u;
    module->descriptor.module_model_id = 1u;
    module->descriptor.hardware_rev_major = 1u;
    module->descriptor.hardware_rev_minor = 0u;
    module->descriptor.firmware_rev_major = 1u;
    module->descriptor.firmware_rev_minor = 0u;
    module->descriptor.firmware_rev_patch = 0u;
    module->descriptor.max_payload_size = 256u;
    module->descriptor.capability_manifest_total_bytes = module->capability_manifest_length;
    module->descriptor.capability_count = 1u;
    return true;
}

bool reference_gpio_module_exchange(
    void *ctx,
    const uint8_t *tx_buffer,
    uint16_t tx_length,
    uint8_t *rx_buffer,
    uint16_t rx_buffer_size,
    uint16_t *out_rx_length)
{
    reference_gpio_module_t *module = (reference_gpio_module_t *)ctx;
    dek_packet_t request_packet;

    if (module == NULL ||
        tx_buffer == NULL ||
        rx_buffer == NULL ||
        out_rx_length == NULL)
    {
        return false;
    }

    if (!dek_transport_receive(&module->transport, &request_packet, tx_buffer, tx_length))
    {
        return false;
    }

    switch ((dek_message_type_t)request_packet.header.message_type)
    {
        case DEK_MSG_HELLO:
            return reference_gpio_module_handle_hello(
                module,
                rx_buffer,
                rx_buffer_size,
                out_rx_length);

        case DEK_MSG_GET_DESCRIPTOR:
            return reference_gpio_module_handle_get_descriptor(
                module,
                rx_buffer,
                rx_buffer_size,
                out_rx_length);

        case DEK_MSG_GET_CAPABILITIES:
            return reference_gpio_module_handle_get_capabilities(
                module,
                &request_packet,
                rx_buffer,
                rx_buffer_size,
                out_rx_length);

        case DEK_MSG_OPEN_CAPABILITY:
            return reference_gpio_module_handle_open_capability(
                module,
                &request_packet,
                rx_buffer,
                rx_buffer_size,
                out_rx_length);

        case DEK_MSG_CLOSE_CAPABILITY:
            return reference_gpio_module_handle_close_capability(
                module,
                &request_packet,
                rx_buffer,
                rx_buffer_size,
                out_rx_length);

        case DEK_MSG_COMMAND:
            return reference_gpio_module_handle_gpio_command(
                module,
                &request_packet,
                rx_buffer,
                rx_buffer_size,
                out_rx_length);

        default:
            return false;
    }
}
