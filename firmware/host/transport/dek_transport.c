#include "dek_transport.h"

/*
 * Reset the transport state before sending any packets.
 *
 * A starting sequence of 1 keeps the first packet distinguishable from an
 * uninitialized or zero-valued counter and preserves a simple monotonic order.
 */
void dek_transport_init(dek_transport_t *transport)
{
    if (transport == NULL)
    {
        return;
    }

    transport->next_sequence_number = 1;
}

/*
 * Encode a packet using the transport-layer metadata.
 *
 * This function fills the packet header with the message type, logical channel,
 * payload length, and a freshly assigned sequence number. The payload bytes are
 * passed through without copying so the caller retains ownership of the data.
 */
bool dek_transport_send(
    dek_transport_t *transport,
    dek_message_type_t message_type,
    uint16_t channel,
    const uint8_t *payload,
    uint16_t payload_length,
    uint8_t *tx_buffer,
    uint16_t tx_buffer_size)
{
    if (transport == NULL || tx_buffer == NULL)
    {
        return false;
    }

    dek_packet_t packet;

    /* Initialize the header to the default protocol values before populating it. */
    dek_packet_init(&packet.header);

    /* Populate the packet header with transport-layer and message-specific fields. */
    packet.header.message_type = message_type;
    packet.header.sequence_number = transport->next_sequence_number++;
    packet.header.channel_id = channel;
    packet.header.payload_length = payload_length;

    /* The payload is referenced directly by the encoder. */
    packet.payload = payload;

    return dek_packet_encode(
        &packet,
        tx_buffer,
        tx_buffer_size);
}