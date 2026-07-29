#include "watch/tseho_link.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t count;
    uint8_t type;
    uint8_t flags;
    uint16_t sequence;
    uint16_t payload_length;
    uint8_t payload[TSEHO_LINK_MAX_PAYLOAD];
} captured_frame_t;

static int bytes_equal(const uint8_t *left,
                       const uint8_t *right,
                       size_t length)
{
    for (size_t index = 0u; index < length; ++index) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

static void capture_frame(const tseho_link_frame_view_t *frame, void *context)
{
    captured_frame_t *captured = (captured_frame_t *)context;

    captured->count++;
    captured->type = frame->type;
    captured->flags = frame->flags;
    captured->sequence = frame->sequence;
    captured->payload_length = frame->payload_length;
    for (uint16_t index = 0u; index < frame->payload_length; ++index) {
        captured->payload[index] = frame->payload[index];
    }
}

static int test_encode_and_split(void)
{
    static const uint8_t payload[] = {
        0x78u, 0x56u, 0x34u, 0x12u, 0xB4u, 0x00u, 0x01u, 0x00u
    };
    static const uint8_t expected[] = {
        0x54u, 0x4Cu, 0x00u, 0x01u, 0x10u, 0x00u, 0x34u, 0x12u,
        0x08u, 0x00u, 0x78u, 0x56u, 0x34u, 0x12u, 0xB4u, 0x00u,
        0x01u, 0x00u
    };
    uint8_t encoded[TSEHO_LINK_MAX_FRAME_SIZE];
    size_t encoded_length = 0u;

    if (tseho_link_encode(TSEHO_LINK_MSG_TIME_SET, 0u, 0x1234u,
                          payload, sizeof(payload),
                          encoded, sizeof(encoded),
                          &encoded_length) != TSEHO_LINK_ENCODE_OK) {
        return 1;
    }
    if ((encoded_length != sizeof(expected)) ||
        !bytes_equal(encoded, expected, sizeof(expected))) {
        return 2;
    }

    for (size_t split = 0u; split <= encoded_length; ++split) {
        tseho_link_parser_t parser;
        captured_frame_t captured = {0};

        tseho_link_parser_init(&parser);
        (void)tseho_link_parser_feed(&parser, encoded, split,
                                     capture_frame, &captured);
        (void)tseho_link_parser_feed(&parser, &encoded[split],
                                     encoded_length - split,
                                     capture_frame, &captured);
        if ((captured.count != 1u) ||
            (captured.type != TSEHO_LINK_MSG_TIME_SET) ||
            (captured.sequence != 0x1234u) ||
            (captured.payload_length != sizeof(payload)) ||
            !bytes_equal(captured.payload, payload, sizeof(payload))) {
            return 3;
        }
    }

    return 0;
}

static int test_noise_concatenation_and_overflow(void)
{
    static const uint8_t ready[] = {
        0x54u, 0x4Cu, 0x00u, 0x01u, 0x02u,
        0x00u, 0x02u, 0x00u, 0x00u, 0x00u
    };
    static const uint8_t stream[] = {
        0x00u, 0x54u, 0x54u, 0x00u, 0x4Cu,
        0x54u, 0x4Cu, 0x00u, 0x01u, 0x02u,
        0x00u, 0x02u, 0x00u, 0x00u, 0x00u,
        0x54u, 0x4Cu, 0x00u, 0x01u, 0x02u,
        0x00u, 0x02u, 0x00u, 0x00u, 0x00u
    };
    static const uint8_t oversized[] = {
        0x54u, 0x4Cu, 0x00u, 0x01u, 0x30u,
        0x00u, 0x01u, 0x00u, 0x81u, 0x01u
    };
    tseho_link_parser_t parser;
    captured_frame_t captured = {0};

    tseho_link_parser_init(&parser);
    if (tseho_link_parser_feed(&parser, stream, sizeof(stream),
                               capture_frame, &captured) != 2u) {
        return 1;
    }
    if ((captured.count != 2u) || (captured.type != TSEHO_LINK_MSG_READY) ||
        (parser.discarded_bytes < 5u)) {
        return 2;
    }
    if (!bytes_equal(&stream[5], ready, sizeof(ready))) {
        return 3;
    }

    (void)tseho_link_parser_feed(&parser, oversized, sizeof(oversized),
                                 capture_frame, &captured);
    if (parser.rejected_headers != 1u) {
        return 4;
    }

    return 0;
}

static int test_maximum_payload(void)
{
    uint8_t payload[TSEHO_LINK_MAX_PAYLOAD];
    uint8_t encoded[TSEHO_LINK_MAX_FRAME_SIZE];
    size_t encoded_length = 0u;
    tseho_link_parser_t parser;
    captured_frame_t captured = {0};

    for (size_t index = 0u; index < sizeof(payload); ++index) {
        payload[index] = (uint8_t)(index ^ 0xA5u);
    }
    if (tseho_link_encode(TSEHO_LINK_MSG_NOTIFY_ADD, 0u, 0xFFFFu,
                          payload, sizeof(payload),
                          encoded, sizeof(encoded),
                          &encoded_length) != TSEHO_LINK_ENCODE_OK) {
        return 1;
    }
    if (encoded_length != TSEHO_LINK_MAX_FRAME_SIZE) {
        return 2;
    }

    tseho_link_parser_init(&parser);
    for (size_t index = 0u; index < encoded_length; ++index) {
        (void)tseho_link_parser_feed(&parser, &encoded[index], 1u,
                                     capture_frame, &captured);
    }
    if ((captured.count != 1u) ||
        (captured.payload_length != TSEHO_LINK_MAX_PAYLOAD) ||
        !bytes_equal(captured.payload, payload, sizeof(payload))) {
        return 3;
    }

    return 0;
}

int main(void)
{
    int result;

    result = test_encode_and_split();
    if (result != 0) {
        return 10 + result;
    }
    result = test_noise_concatenation_and_overflow();
    if (result != 0) {
        return 20 + result;
    }
    result = test_maximum_payload();
    if (result != 0) {
        return 30 + result;
    }
    return 0;
}
