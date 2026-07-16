#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "transport/dek_packet.h"

static bool expect_true(const char *label, bool condition)
{
    if (!condition) {
        printf("FAIL: %s\n", label);
        return false;
    }

    printf("PASS: %s\n", label);
    return true;
}

static bool expect_u8(const char *label, uint8_t actual, uint8_t expected)
{
    if (actual != expected) {
        printf("FAIL: %s (expected=%u actual=%u)\n", label, expected, actual);
        return false;
    }

    printf("PASS: %s\n", label);
    return true;
}

static bool expect_u16(const char *label, uint16_t actual, uint16_t expected)
{
    if (actual != expected) {
        printf("FAIL: %s (expected=%u actual=%u)\n", label, expected, actual);
        return false;
    }

    printf("PASS: %s\n", label);
    return true;
}

static bool test_init_defaults(void)
{
    dek_packet_header_t header;

    memset(&header, 0xA5, sizeof(header));
    dek_packet_init(&header);

    return expect_u8("init magic[0]", header.magic[0], DEK_PACKET_MAGIC_BYTE0) &&
        expect_u8("init magic[1]", header.magic[1], DEK_PACKET_MAGIC_BYTE1) &&
        expect_u8("init protocol version", header.protocol_version, DEK_PACKET_VERSION_V1) &&
        expect_u8("init message type", header.message_type, 0) &&
        expect_u8("init flags", header.flags, 0) &&
        expect_u8("init header length", header.header_length, DEK_PACKET_HEADER_SIZE) &&
        expect_u16("init sequence number", header.sequence_number, 0) &&
        expect_u16("init channel id", header.channel_id, 0) &&
        expect_u16("init payload length", header.payload_length, 0);
}

static bool test_encode_decode_round_trip(void)
{
    dek_packet_header_t source_header;
    dek_packet_header_t decoded_header;
    uint8_t buffer[DEK_PACKET_HEADER_SIZE];

    dek_packet_init(&source_header);
    source_header.message_type = 0x07;
    source_header.flags = 0x09;
    source_header.sequence_number = 0x1234;
    source_header.channel_id = 0x0042;
    source_header.payload_length = 0x0020;

    memset(&decoded_header, 0, sizeof(decoded_header));
    memset(buffer, 0, sizeof(buffer));

    if (!expect_true(
            "encode header succeeds",
            dek_packet_encode_header(&source_header, buffer, sizeof(buffer)))) {
        return false;
    }

    if (!expect_true(
            "decode header succeeds",
            dek_packet_decode_header(&decoded_header, buffer, sizeof(buffer)))) {
        return false;
    }

    return expect_u8("decode magic[0]", decoded_header.magic[0], source_header.magic[0]) &&
        expect_u8("decode magic[1]", decoded_header.magic[1], source_header.magic[1]) &&
        expect_u8("decode protocol version", decoded_header.protocol_version, source_header.protocol_version) &&
        expect_u8("decode message type", decoded_header.message_type, source_header.message_type) &&
        expect_u8("decode flags", decoded_header.flags, source_header.flags) &&
        expect_u8("decode header length", decoded_header.header_length, source_header.header_length) &&
        expect_u16("decode sequence number", decoded_header.sequence_number, source_header.sequence_number) &&
        expect_u16("decode channel id", decoded_header.channel_id, source_header.channel_id) &&
        expect_u16("decode payload length", decoded_header.payload_length, source_header.payload_length);
}

static bool test_invalid_decode_rejected(void)
{
    dek_packet_header_t header;
    uint8_t buffer[DEK_PACKET_HEADER_SIZE];

    memset(&header, 0, sizeof(header));
    memset(buffer, 0, sizeof(buffer));

    buffer[DEK_PACKET_OFFSET_MAGIC] = 0x00;
    buffer[DEK_PACKET_OFFSET_MAGIC + 1] = 0x00;

    return expect_true(
        "decode rejects invalid magic",
        !dek_packet_decode_header(&header, buffer, sizeof(buffer)));
}

void dek_host_app_main(void)
{
    bool all_passed;

    printf("Running dek_packet unit tests...\n");

    all_passed = test_init_defaults() &&
        test_encode_decode_round_trip() &&
        test_invalid_decode_rejected();

    if (all_passed) {
        printf("dek_packet unit tests passed.\n");
        return;
    }

    printf("dek_packet unit tests failed.\n");
}
