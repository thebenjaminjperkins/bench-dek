
#include "dek_packet.h"

void dek_packet_init(dek_packet_header_t *header)
{
    if (header == NULL)
    {
        return;
    }

    header->magic[0] = DEK_PACKET_MAGIC_BYTE0;
    header->magic[1] = DEK_PACKET_MAGIC_BYTE1;
    header->protocol_version = DEK_PACKET_VERSION_V1;
    header->message_type = 0;
    header->flags = 0;
    header->header_length = DEK_PACKET_HEADER_SIZE;
    header->sequence_number = 0;
    header->channel_id = 0;
    header->payload_length = 0;
}


bool dek_packet_encode_header(
    const dek_packet_header_t *header,
    uint8_t *buffer,
    uint16_t buffer_size)
{
    if (header == NULL || buffer == NULL || buffer_size < DEK_PACKET_HEADER_SIZE)
    {
        return false;
    }

    buffer[DEK_PACKET_OFFSET_MAGIC] = header->magic[0];
    buffer[DEK_PACKET_OFFSET_MAGIC + 1] = header->magic[1];
    buffer[DEK_PACKET_OFFSET_PROTOCOL_VERSION] = header->protocol_version;
    buffer[DEK_PACKET_OFFSET_MESSAGE_TYPE] = header->message_type;
    buffer[DEK_PACKET_OFFSET_FLAGS] = header->flags;
    buffer[DEK_PACKET_OFFSET_HEADER_LENGTH] = header->header_length;

    buffer[DEK_PACKET_OFFSET_SEQUENCE_NUMBER] = header->sequence_number & 0xFF;
    buffer[DEK_PACKET_OFFSET_SEQUENCE_NUMBER + 1] = (header->sequence_number >> 8) & 0xFF;

    buffer[DEK_PACKET_OFFSET_CHANNEL_ID] = header->channel_id & 0xFF;
    buffer[DEK_PACKET_OFFSET_CHANNEL_ID + 1] = (header->channel_id >> 8) & 0xFF;

    buffer[DEK_PACKET_OFFSET_PAYLOAD_LENGTH] = header->payload_length & 0xFF;
    buffer[DEK_PACKET_OFFSET_PAYLOAD_LENGTH + 1] = (header->payload_length >> 8) & 0xFF;

    return true;
}

bool dek_packet_decode_header(
    dek_packet_header_t *header,
    const uint8_t *buffer,
    uint16_t buffer_size)
{
    if (header == NULL || buffer == NULL || buffer_size < DEK_PACKET_HEADER_SIZE)
    {
        return false;
    }

    if (buffer[DEK_PACKET_OFFSET_MAGIC] != DEK_PACKET_MAGIC_BYTE0 ||
        buffer[DEK_PACKET_OFFSET_MAGIC + 1] != DEK_PACKET_MAGIC_BYTE1)
    {
        return false;
    }

    header->magic[0] = buffer[DEK_PACKET_OFFSET_MAGIC];
    header->magic[1] = buffer[DEK_PACKET_OFFSET_MAGIC + 1];
    header->protocol_version = buffer[DEK_PACKET_OFFSET_PROTOCOL_VERSION];
    header->message_type = buffer[DEK_PACKET_OFFSET_MESSAGE_TYPE];
    header->flags = buffer[DEK_PACKET_OFFSET_FLAGS];
    header->header_length = buffer[DEK_PACKET_OFFSET_HEADER_LENGTH];

    header->sequence_number =
    (uint16_t)buffer[DEK_PACKET_OFFSET_SEQUENCE_NUMBER] |
    ((uint16_t)buffer[DEK_PACKET_OFFSET_SEQUENCE_NUMBER + 1] << 8);

    header->channel_id = (uint16_t)buffer[DEK_PACKET_OFFSET_CHANNEL_ID] | 
    ((uint16_t)buffer[DEK_PACKET_OFFSET_CHANNEL_ID + 1] << 8);

    header->payload_length = (uint16_t)buffer[DEK_PACKET_OFFSET_PAYLOAD_LENGTH] | 
    ((uint16_t)buffer[DEK_PACKET_OFFSET_PAYLOAD_LENGTH + 1] << 8);

    return true;
}