#ifndef WATCH_BLE_H
#define WATCH_BLE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    WATCH_BLE_STATE_OFF = 0,
    WATCH_BLE_STATE_ADVERTISING = 1,
    WATCH_BLE_STATE_CONNECTED = 2,
    WATCH_BLE_STATE_SUBSCRIBED = 3,
    WATCH_BLE_STATE_READY = 4,
    WATCH_BLE_STATE_ERROR = 0xFF
} watch_ble_state_t;

typedef struct {
    uint32_t state;
    uint32_t error;
    uint32_t required_ram_start;
    uint32_t connections;
    uint32_t received_packets;
    uint32_t received_bytes;
    uint32_t last_packet_header;
    uint32_t tx_notifications;
    uint32_t scan_reports;
    int32_t scan_last_rssi;
    uint32_t scan_last_address;
} watch_ble_status_t;

typedef struct {
    uint8_t state;
    uint8_t volume;
    uint8_t shuffle;
    uint8_t repeat;
    uint32_t duration_s;
    uint32_t position_s;
    char artist[49];
    char album[49];
    char track[97];
    char source_app[33];
    uint32_t revision;
} watch_ble_media_status_t;

enum {
    WATCH_NOTIFICATION_CAPACITY = 6,
    WATCH_NOTIFICATION_APP_BYTES = 25,
    WATCH_NOTIFICATION_TITLE_BYTES = 65,
    WATCH_NOTIFICATION_BODY_BYTES = 129
};

typedef struct {
    uint32_t id;
    uint8_t category;
    char app[WATCH_NOTIFICATION_APP_BYTES];
    char title[WATCH_NOTIFICATION_TITLE_BYTES];
    char body[WATCH_NOTIFICATION_BODY_BYTES];
} watch_ble_notification_t;

typedef struct {
    watch_ble_notification_t items[WATCH_NOTIFICATION_CAPACITY];
    uint8_t count;
    uint32_t revision;
} watch_ble_notifications_t;

typedef struct {
    const uint8_t *pixels;
    uint16_t width;
    uint16_t height;
    uint32_t revision;
    bool valid;
} watch_ble_artwork_status_t;

bool watch_ble_init(void);
void watch_ble_poll(void);
const watch_ble_status_t *watch_ble_get_status(void);
const watch_ble_media_status_t *watch_ble_get_media_status(void);
const watch_ble_notifications_t *watch_ble_get_notifications(void);
const watch_ble_artwork_status_t *watch_ble_get_artwork_status(void);
bool watch_ble_send_media_command(uint8_t command);

#endif
