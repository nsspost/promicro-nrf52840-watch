#ifndef WATCH_TSEHO_LINK_H
#define WATCH_TSEHO_LINK_H

#include <stddef.h>
#include <stdint.h>

#define TSEHO_LINK_MAGIC_0             0x54u
#define TSEHO_LINK_MAGIC_1             0x4Cu
#define TSEHO_LINK_VERSION_MAJOR       0u
#define TSEHO_LINK_VERSION_MINOR       1u
#define TSEHO_LINK_HEADER_SIZE         10u
#define TSEHO_LINK_MAX_PAYLOAD         384u
#define TSEHO_LINK_MAX_FRAME_SIZE      \
    (TSEHO_LINK_HEADER_SIZE + TSEHO_LINK_MAX_PAYLOAD)

#define TSEHO_LINK_FLAG_ACK_REQUIRED   0x01u
#define TSEHO_LINK_FLAG_RESPONSE       0x02u
#define TSEHO_LINK_KNOWN_FLAGS         \
    (TSEHO_LINK_FLAG_ACK_REQUIRED | TSEHO_LINK_FLAG_RESPONSE)

#define TSEHO_LINK_CAP_TIME            (1u << 0)
#define TSEHO_LINK_CAP_WATCH_BATTERY   (1u << 1)
#define TSEHO_LINK_CAP_NOTIFICATIONS   (1u << 2)
#define TSEHO_LINK_CAP_MEDIA_INFO      (1u << 3)
#define TSEHO_LINK_CAP_MEDIA_CONTROL   (1u << 4)

typedef enum {
    TSEHO_LINK_MSG_HELLO = 0x01,
    TSEHO_LINK_MSG_READY = 0x02,
    TSEHO_LINK_MSG_ACK = 0x03,
    TSEHO_LINK_MSG_TIME_SET = 0x10,
    TSEHO_LINK_MSG_WATCH_BATTERY = 0x20,
    TSEHO_LINK_MSG_NOTIFY_ADD = 0x30,
    TSEHO_LINK_MSG_NOTIFY_REMOVE = 0x31,
    TSEHO_LINK_MSG_MEDIA_INFO = 0x40,
    TSEHO_LINK_MSG_MEDIA_STATE = 0x41,
    TSEHO_LINK_MSG_MEDIA_VOLUME = 0x42,
    TSEHO_LINK_MSG_MEDIA_COMMAND = 0x43,
    TSEHO_LINK_MSG_ERROR = 0x7F
} tseho_link_message_type_t;

typedef enum {
    TSEHO_LINK_ENCODE_OK = 0,
    TSEHO_LINK_ENCODE_INVALID_ARGUMENT,
    TSEHO_LINK_ENCODE_PAYLOAD_TOO_LARGE,
    TSEHO_LINK_ENCODE_OUTPUT_TOO_SMALL
} tseho_link_encode_result_t;

typedef struct {
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t type;
    uint8_t flags;
    uint16_t sequence;
    uint16_t payload_length;
    const uint8_t *payload;
} tseho_link_frame_view_t;

typedef void (*tseho_link_frame_handler_t)(
    const tseho_link_frame_view_t *frame,
    void *context);

typedef struct {
    uint8_t frame[TSEHO_LINK_MAX_FRAME_SIZE];
    uint16_t used;
    uint16_t expected;
    uint32_t accepted_frames;
    uint32_t rejected_headers;
    uint32_t discarded_bytes;
} tseho_link_parser_t;

void tseho_link_parser_init(tseho_link_parser_t *parser);

size_t tseho_link_parser_feed(tseho_link_parser_t *parser,
                              const uint8_t *data,
                              size_t length,
                              tseho_link_frame_handler_t handler,
                              void *context);

tseho_link_encode_result_t tseho_link_encode(
    uint8_t type,
    uint8_t flags,
    uint16_t sequence,
    const uint8_t *payload,
    uint16_t payload_length,
    uint8_t *output,
    size_t output_capacity,
    size_t *output_length);

uint16_t tseho_link_read_u16_le(const uint8_t *data);
uint32_t tseho_link_read_u32_le(const uint8_t *data);
void tseho_link_write_u16_le(uint8_t *data, uint16_t value);
void tseho_link_write_u32_le(uint8_t *data, uint32_t value);

#endif
