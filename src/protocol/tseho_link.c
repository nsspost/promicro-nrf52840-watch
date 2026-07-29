#include "watch/tseho_link.h"

static void tseho_link_parser_reset_frame(tseho_link_parser_t *parser)
{
    parser->used = 0u;
    parser->expected = TSEHO_LINK_HEADER_SIZE;
}

uint16_t tseho_link_read_u16_le(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] |
                      ((uint16_t)data[1] << 8));
}

uint32_t tseho_link_read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

void tseho_link_write_u16_le(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

void tseho_link_write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

void tseho_link_parser_init(tseho_link_parser_t *parser)
{
    if (parser == NULL) {
        return;
    }

    parser->accepted_frames = 0u;
    parser->rejected_headers = 0u;
    parser->discarded_bytes = 0u;
    tseho_link_parser_reset_frame(parser);
}

static void tseho_link_parser_begin_magic(tseho_link_parser_t *parser)
{
    parser->frame[0] = TSEHO_LINK_MAGIC_0;
    parser->used = 1u;
}

static void tseho_link_parser_accept_search_byte(tseho_link_parser_t *parser,
                                                  uint8_t value)
{
    if (value == TSEHO_LINK_MAGIC_0) {
        tseho_link_parser_begin_magic(parser);
    } else {
        parser->discarded_bytes++;
    }
}

static void tseho_link_parser_accept_second_magic(
    tseho_link_parser_t *parser,
    uint8_t value)
{
    if (value == TSEHO_LINK_MAGIC_1) {
        parser->frame[1] = value;
        parser->used = 2u;
    } else if (value == TSEHO_LINK_MAGIC_0) {
        parser->discarded_bytes++;
        tseho_link_parser_begin_magic(parser);
    } else {
        parser->discarded_bytes += 2u;
        tseho_link_parser_reset_frame(parser);
    }
}

static int tseho_link_parser_header_ready(tseho_link_parser_t *parser)
{
    uint16_t payload_length =
        tseho_link_read_u16_le(&parser->frame[8]);

    if (payload_length > TSEHO_LINK_MAX_PAYLOAD) {
        parser->rejected_headers++;
        parser->discarded_bytes += TSEHO_LINK_HEADER_SIZE;
        tseho_link_parser_reset_frame(parser);
        return 0;
    }

    parser->expected =
        (uint16_t)(TSEHO_LINK_HEADER_SIZE + payload_length);
    return 1;
}

static void tseho_link_parser_deliver(tseho_link_parser_t *parser,
                                      tseho_link_frame_handler_t handler,
                                      void *context)
{
    tseho_link_frame_view_t view = {
        .version_major = parser->frame[2],
        .version_minor = parser->frame[3],
        .type = parser->frame[4],
        .flags = parser->frame[5],
        .sequence = tseho_link_read_u16_le(&parser->frame[6]),
        .payload_length = tseho_link_read_u16_le(&parser->frame[8]),
        .payload = &parser->frame[TSEHO_LINK_HEADER_SIZE]
    };

    handler(&view, context);
    parser->accepted_frames++;
    tseho_link_parser_reset_frame(parser);
}

size_t tseho_link_parser_feed(tseho_link_parser_t *parser,
                              const uint8_t *data,
                              size_t length,
                              tseho_link_frame_handler_t handler,
                              void *context)
{
    size_t delivered_before;

    if ((parser == NULL) || (handler == NULL) ||
        ((data == NULL) && (length != 0u))) {
        return 0u;
    }

    delivered_before = parser->accepted_frames;

    for (size_t index = 0u; index < length; ++index) {
        uint8_t value = data[index];

        if (parser->used == 0u) {
            tseho_link_parser_accept_search_byte(parser, value);
            continue;
        }
        if (parser->used == 1u) {
            tseho_link_parser_accept_second_magic(parser, value);
            continue;
        }

        parser->frame[parser->used++] = value;

        if (parser->used == TSEHO_LINK_HEADER_SIZE) {
            if (!tseho_link_parser_header_ready(parser)) {
                continue;
            }
        }
        if (parser->used == parser->expected) {
            tseho_link_parser_deliver(parser, handler, context);
        }
    }

    return (size_t)(parser->accepted_frames - delivered_before);
}

tseho_link_encode_result_t tseho_link_encode(
    uint8_t type,
    uint8_t flags,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t payload_length,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length)
{
    size_t frame_length;

    if ((output == NULL) || (output_length == NULL) ||
        ((payload == NULL) && (payload_length != 0u))) {
        return TSEHO_LINK_ENCODE_INVALID_ARGUMENT;
    }
    if (payload_length > TSEHO_LINK_MAX_PAYLOAD) {
        return TSEHO_LINK_ENCODE_PAYLOAD_TOO_LARGE;
    }

    frame_length = TSEHO_LINK_HEADER_SIZE + payload_length;
    if (output_capacity < frame_length) {
        return TSEHO_LINK_ENCODE_OUTPUT_TOO_SMALL;
    }

    output[0] = TSEHO_LINK_MAGIC_0;
    output[1] = TSEHO_LINK_MAGIC_1;
    output[2] = TSEHO_LINK_VERSION_MAJOR;
    output[3] = TSEHO_LINK_VERSION_MINOR;
    output[4] = type;
    output[5] = flags;
    tseho_link_write_u16_le(&output[6], sequence);
    tseho_link_write_u16_le(&output[8], payload_length);

    for (uint16_t index = 0u; index < payload_length; ++index) {
        output[TSEHO_LINK_HEADER_SIZE + index] = payload[index];
    }

    *output_length = frame_length;
    return TSEHO_LINK_ENCODE_OK;
}
