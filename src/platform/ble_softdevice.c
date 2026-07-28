#include "watch/ble.h"

#include <stddef.h>
#include <stdint.h>

#include "ble.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "nrf_error.h"
#include "nrf_sdm.h"
#include "watch/clock.h"
#include "watch/tseho_link.h"

#define WATCH_BLE_RAM_START       0x20004000u
#define WATCH_BLE_EVENT_BUFFER    256u
#define WATCH_BLE_CONN_TAG        1u
#define WATCH_BLE_VALUE_MAX_LEN   20u

#define WATCH_LINK_SERVICE_UUID   0x0001u
#define WATCH_LINK_RX_UUID        0x0002u
#define WATCH_LINK_TX_UUID        0x0003u

static const uint8_t watch_device_name[] = "Tseho Watch";
static const ble_uuid128_t watch_link_base_uuid = {
    .uuid128 = {
        0x2E, 0x73, 0xB5, 0xF0, 0x91, 0x6C, 0xD1, 0xA2,
        0x8B, 0x4E, 0xF7, 0x34, 0x00, 0x00, 0x5C, 0x7A
    }
};

static uint8_t watch_adv_data[] = {
    2u, BLE_GAP_AD_TYPE_FLAGS,
    BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
    sizeof(watch_device_name), BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
    'T', 's', 'e', 'h', 'o', ' ', 'W', 'a', 't', 'c', 'h'
};

static uint8_t watch_scan_response[] = {
    17u, BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE,
    0x2E, 0x73, 0xB5, 0xF0, 0x91, 0x6C, 0xD1, 0xA2,
    0x8B, 0x4E, 0xF7, 0x34, 0x01, 0x00, 0x5C, 0x7A
};

static __attribute__((aligned(4))) uint8_t watch_ble_event[WATCH_BLE_EVENT_BUFFER];
static watch_ble_status_t watch_ble_status;
static watch_ble_media_status_t watch_ble_media = {
    .volume = 0xFFu, .shuffle = 0xFFu, .repeat = 0xFFu
};
static ble_gatts_char_handles_t watch_link_rx_handles;
static ble_gatts_char_handles_t watch_link_tx_handles;
static uint16_t watch_conn_handle = BLE_CONN_HANDLE_INVALID;
static uint8_t watch_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static uint8_t watch_link_uuid_type;
static bool watch_tx_subscribed;
static bool watch_hello_pending;
static uint16_t watch_link_tx_sequence;
static tseho_link_parser_t watch_link_parser;

static void watch_softdevice_fault(uint32_t id, uint32_t pc, uint32_t info)
{
    watch_ble_status.state = WATCH_BLE_STATE_ERROR;
    watch_ble_status.error = 0xF0000000u | (id & 0xFFFFu);
    watch_ble_status.last_packet_header = pc ^ info;
    for (;;) {
        __asm volatile ("wfi");
    }
}

static bool watch_ble_check(uint32_t result, uint32_t step)
{
    if (result == NRF_SUCCESS) {
        return true;
    }
    watch_ble_status.state = WATCH_BLE_STATE_ERROR;
    watch_ble_status.error = (step << 24) | (result & 0x00FFFFFFu);
    return false;
}

static void watch_open_security(ble_gap_conn_sec_mode_t *mode)
{
    mode->sm = 1u;
    mode->lv = 1u;
}

static bool watch_link_add_characteristic(uint16_t service_handle,
                                          uint16_t uuid_value,
                                          bool rx,
                                          ble_gatts_char_handles_t *handles)
{
    uint8_t initial_value = 0u;
    ble_uuid_t uuid = {
        .uuid = uuid_value,
        .type = watch_link_uuid_type
    };
    ble_gatts_attr_md_t value_md = {0};
    ble_gatts_attr_t value_attr = {0};
    ble_gatts_char_md_t char_md = {0};
    ble_gatts_attr_md_t cccd_md = {0};

    if (rx) {
        watch_open_security(&value_md.read_perm);
        watch_open_security(&value_md.write_perm);
    }
    value_md.vloc = BLE_GATTS_VLOC_STACK;
    value_md.vlen = 1u;

    value_attr.p_uuid = &uuid;
    value_attr.p_attr_md = &value_md;
    value_attr.init_len = 1u;
    value_attr.max_len = WATCH_BLE_VALUE_MAX_LEN;
    value_attr.p_value = &initial_value;

    if (rx) {
        char_md.char_props.write = 1u;
        char_md.char_props.write_wo_resp = 1u;
    } else {
        char_md.char_props.notify = 1u;
        watch_open_security(&cccd_md.read_perm);
        watch_open_security(&cccd_md.write_perm);
        cccd_md.vloc = BLE_GATTS_VLOC_STACK;
        char_md.p_cccd_md = &cccd_md;
    }

    return watch_ble_check(
        sd_ble_gatts_characteristic_add(service_handle,
                                        &char_md,
                                        &value_attr,
                                        handles),
        rx ? 7u : 8u);
}

static bool watch_link_init(void)
{
    ble_uuid_t service_uuid = {0};
    uint16_t service_handle;

    if (!watch_ble_check(sd_ble_uuid_vs_add(&watch_link_base_uuid,
                                            &watch_link_uuid_type), 5u)) {
        return false;
    }
    service_uuid.type = watch_link_uuid_type;
    service_uuid.uuid = WATCH_LINK_SERVICE_UUID;
    if (!watch_ble_check(
            sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                     &service_uuid,
                                     &service_handle),
            6u)) {
        return false;
    }
    if (!watch_link_add_characteristic(service_handle,
                                       WATCH_LINK_RX_UUID, true,
                                       &watch_link_rx_handles)) {
        return false;
    }
    return watch_link_add_characteristic(service_handle,
                                         WATCH_LINK_TX_UUID, false,
                                         &watch_link_tx_handles);
}

static bool watch_link_notify(uint8_t type,
                              uint8_t flags,
                              const uint8_t *payload,
                              uint16_t payload_length)
{
    uint8_t frame[WATCH_BLE_VALUE_MAX_LEN];
    size_t frame_length = 0u;
    uint16_t hvx_length;
    ble_gatts_hvx_params_t params = {0};

    if (!watch_tx_subscribed ||
        (watch_conn_handle == BLE_CONN_HANDLE_INVALID)) {
        return false;
    }
    if (tseho_link_encode(type, flags, watch_link_tx_sequence,
                          payload, payload_length,
                          frame, sizeof(frame), &frame_length) !=
        TSEHO_LINK_ENCODE_OK) {
        return false;
    }

    hvx_length = (uint16_t)frame_length;
    params.handle = watch_link_tx_handles.value_handle;
    params.type = BLE_GATT_HVX_NOTIFICATION;
    params.offset = 0u;
    params.p_len = &hvx_length;
    params.p_data = frame;
    uint32_t result = sd_ble_gatts_hvx(watch_conn_handle, &params);
    if (result == NRF_SUCCESS) {
        watch_link_tx_sequence++;
        watch_ble_status.tx_notifications++;
        return true;
    }
    if (result != NRF_ERROR_RESOURCES) {
        watch_ble_status.error = (17u << 24) | result;
    }
    return false;
}

static bool watch_link_send_hello(void)
{
    uint8_t payload[8];
    uint32_t capabilities =
        TSEHO_LINK_CAP_TIME |
        TSEHO_LINK_CAP_WATCH_BATTERY |
        TSEHO_LINK_CAP_NOTIFICATIONS |
        TSEHO_LINK_CAP_MEDIA_INFO |
        TSEHO_LINK_CAP_MEDIA_CONTROL;

    tseho_link_write_u32_le(&payload[0], capabilities);
    tseho_link_write_u16_le(&payload[4], TSEHO_LINK_MAX_PAYLOAD);
    tseho_link_write_u16_le(&payload[6], WATCH_BLE_VALUE_MAX_LEN);
    return watch_link_notify(TSEHO_LINK_MSG_HELLO,
                             TSEHO_LINK_FLAG_ACK_REQUIRED,
                             payload, sizeof(payload));
}

static void watch_link_apply_time(const uint8_t *payload,
                                  uint16_t payload_length)
{
    if (payload_length != 8u) {
        return;
    }

    uint32_t utc_day_seconds =
        tseho_link_read_u32_le(payload) % 86400u;
    int16_t offset_minutes =
        (int16_t)tseho_link_read_u16_le(&payload[4]);
    int32_t local_seconds =
        (int32_t)utc_day_seconds + ((int32_t)offset_minutes * 60);

    while (local_seconds < 0) {
        local_seconds += 86400;
    }
    while (local_seconds >= 86400) {
        local_seconds -= 86400;
    }
    watch_clock_init((uint8_t)((uint32_t)local_seconds / 3600u),
                     (uint8_t)(((uint32_t)local_seconds % 3600u) / 60u),
                     (uint8_t)((uint32_t)local_seconds % 60u));
}

static void watch_link_copy_text(char *dst, uint16_t capacity,
                                 const uint8_t *src, uint8_t length)
{
    uint16_t count = length < capacity ? length : (uint16_t)(capacity - 1u);
    for (uint16_t i = 0u; i < count; ++i) dst[i] = (char)src[i];
    dst[count] = '\0';
}

static void watch_link_apply_media_info(const uint8_t *payload, uint16_t length)
{
    if ((payload == NULL) || (length < 11u)) return;
    uint8_t artist = payload[8], album = payload[9], track = payload[10];
    uint16_t offset = 11u;
    if ((uint32_t)offset + artist + album + track > length) return;
    watch_ble_media.duration_s = tseho_link_read_u32_le(payload);
    watch_ble_media.position_s = tseho_link_read_u32_le(&payload[4]);
    watch_link_copy_text(watch_ble_media.artist, sizeof(watch_ble_media.artist),
                         &payload[offset], artist);
    offset = (uint16_t)(offset + artist);
    watch_link_copy_text(watch_ble_media.album, sizeof(watch_ble_media.album),
                         &payload[offset], album);
    offset = (uint16_t)(offset + album);
    watch_link_copy_text(watch_ble_media.track, sizeof(watch_ble_media.track),
                         &payload[offset], track);
    ++watch_ble_media.revision;
}

static void watch_link_apply_media_state(const uint8_t *payload, uint16_t length)
{
    if ((payload == NULL) || (length < 8u)) return;
    watch_ble_media.state = payload[0];
    watch_ble_media.shuffle = payload[1];
    watch_ble_media.repeat = payload[2];
    watch_ble_media.position_s = tseho_link_read_u32_le(&payload[4]);
    ++watch_ble_media.revision;
}

static void watch_link_apply_media_volume(const uint8_t *payload, uint16_t length)
{
    if ((payload == NULL) || (length < 1u)) return;
    watch_ble_media.volume = payload[0];
    ++watch_ble_media.revision;
}

static void watch_link_handle_frame(
    const tseho_link_frame_view_t *frame,
    void *context)
{
    (void)context;

    if (frame->version_major != TSEHO_LINK_VERSION_MAJOR) {
        return;
    }
    if (frame->type == TSEHO_LINK_MSG_READY) {
        watch_ble_status.state = WATCH_BLE_STATE_READY;
    } else if (frame->type == TSEHO_LINK_MSG_TIME_SET) {
        watch_link_apply_time(frame->payload, frame->payload_length);
    } else if (frame->type == TSEHO_LINK_MSG_MEDIA_INFO) {
        watch_link_apply_media_info(frame->payload, frame->payload_length);
    } else if (frame->type == TSEHO_LINK_MSG_MEDIA_STATE) {
        watch_link_apply_media_state(frame->payload, frame->payload_length);
    } else if (frame->type == TSEHO_LINK_MSG_MEDIA_VOLUME) {
        watch_link_apply_media_volume(frame->payload, frame->payload_length);
    }
}

static bool watch_advertising_start(bool configure)
{
    if (configure) {
        ble_gap_adv_data_t data = {
            .adv_data = {
                .p_data = watch_adv_data,
                .len = sizeof(watch_adv_data)
            },
            .scan_rsp_data = {
                .p_data = watch_scan_response,
                .len = sizeof(watch_scan_response)
            }
        };
        ble_gap_adv_params_t params = {0};
        params.properties.type =
            BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
        /* 20 ms during RF bring-up: maximize discovery opportunities. */
        params.interval = 32u;
        params.duration = 0u;
        params.filter_policy = BLE_GAP_ADV_FP_ANY;
        params.primary_phy = BLE_GAP_PHY_1MBPS;
        if (!watch_ble_check(
                sd_ble_gap_adv_set_configure(&watch_adv_handle,
                                             &data, &params),
                10u)) {
            return false;
        }
        if (!watch_ble_check(
                sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV,
                                        watch_adv_handle, 8),
                16u)) {
            return false;
        }
    }

    if (!watch_ble_check(sd_ble_gap_adv_start(watch_adv_handle,
                                              WATCH_BLE_CONN_TAG), 11u)) {
        return false;
    }
    watch_ble_status.state = WATCH_BLE_STATE_ADVERTISING;
    return true;
}

bool watch_ble_init(void)
{
    nrf_clock_lf_cfg_t clock_cfg = {
        .source = NRF_CLOCK_LF_SRC_RC,
        .rc_ctiv = 16u,
        .rc_temp_ctiv = 2u,
        .accuracy = NRF_CLOCK_LF_ACCURACY_500_PPM
    };
    ble_cfg_t cfg = {0};
    uint32_t ram_start = WATCH_BLE_RAM_START;
    ble_gap_conn_sec_mode_t name_security = {0};
    ble_gap_conn_params_t conn_params = {
        .min_conn_interval = 24u,
        .max_conn_interval = 40u,
        .slave_latency = 0u,
        .conn_sup_timeout = 400u
    };

    watch_ble_status = (watch_ble_status_t){0};
    watch_conn_handle = BLE_CONN_HANDLE_INVALID;
    watch_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
    watch_tx_subscribed = false;
    watch_hello_pending = false;
    watch_link_tx_sequence = 1u;
    tseho_link_parser_init(&watch_link_parser);

    if (!watch_ble_check(sd_softdevice_enable(&clock_cfg,
                                               watch_softdevice_fault), 1u)) {
        return false;
    }

    cfg.conn_cfg.conn_cfg_tag = WATCH_BLE_CONN_TAG;
    cfg.conn_cfg.params.gap_conn_cfg.conn_count = 1u;
    cfg.conn_cfg.params.gap_conn_cfg.event_length = 6u;
    if (!watch_ble_check(sd_ble_cfg_set(BLE_CONN_CFG_GAP, &cfg,
                                        ram_start), 2u)) {
        return false;
    }

    cfg = (ble_cfg_t){0};
    cfg.gap_cfg.role_count_cfg.adv_set_count = 1u;
    cfg.gap_cfg.role_count_cfg.periph_role_count = 1u;
    cfg.gap_cfg.role_count_cfg.central_role_count = 0u;
    if (!watch_ble_check(sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &cfg,
                                        ram_start), 3u)) {
        return false;
    }

    cfg = (ble_cfg_t){0};
    cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 1u;
    if (!watch_ble_check(sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &cfg,
                                        ram_start), 4u)) {
        return false;
    }

    if (!watch_ble_check(sd_ble_enable(&ram_start), 9u)) {
        watch_ble_status.required_ram_start = ram_start;
        return false;
    }
    watch_ble_status.required_ram_start = ram_start;

    watch_open_security(&name_security);
    if (!watch_ble_check(sd_ble_gap_device_name_set(
            &name_security, watch_device_name,
            sizeof(watch_device_name) - 1u), 12u)) {
        return false;
    }
    if (!watch_ble_check(sd_ble_gap_ppcp_set(&conn_params), 13u)) {
        return false;
    }
    if (!watch_link_init()) {
        return false;
    }
    if (!watch_advertising_start(true)) {
        return false;
    }
    return true;
}

static void watch_ble_process_event(const ble_evt_t *event)
{
    uint16_t event_id = event->header.evt_id;

    if (event_id == BLE_GAP_EVT_CONNECTED) {
        watch_conn_handle = event->evt.gap_evt.conn_handle;
        watch_ble_status.connections++;
        watch_ble_status.state = WATCH_BLE_STATE_CONNECTED;
        (void)sd_ble_gatts_sys_attr_set(watch_conn_handle, NULL, 0u, 0u);
    } else if (event_id == BLE_GAP_EVT_DISCONNECTED) {
        watch_conn_handle = BLE_CONN_HANDLE_INVALID;
        watch_tx_subscribed = false;
        watch_hello_pending = false;
        tseho_link_parser_init(&watch_link_parser);
        (void)watch_advertising_start(false);
    } else if (event_id == BLE_GAP_EVT_SEC_PARAMS_REQUEST) {
        (void)sd_ble_gap_sec_params_reply(
            watch_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
            NULL, NULL);
    } else if (event_id == BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST) {
        (void)sd_ble_gatts_exchange_mtu_reply(watch_conn_handle,
                                              BLE_GATT_ATT_MTU_DEFAULT);
    } else if (event_id == BLE_GATTS_EVT_WRITE) {
        const ble_gatts_evt_write_t *write =
            &event->evt.gatts_evt.params.write;
        if ((write->handle == watch_link_tx_handles.cccd_handle) &&
            (write->len == 2u)) {
            bool was_subscribed = watch_tx_subscribed;
            watch_tx_subscribed = (write->data[0] & 1u) != 0u;
            /* Gadgetbridge may write the CCCD more than once while it
             * rebuilds its notification transaction.  HELLO belongs to the
             * subscription edge, not to every identical CCCD write. */
            if (watch_tx_subscribed && !was_subscribed) {
                watch_hello_pending = true;
            } else if (!watch_tx_subscribed) {
                watch_hello_pending = false;
            }
            watch_ble_status.state = watch_tx_subscribed ?
                WATCH_BLE_STATE_SUBSCRIBED : WATCH_BLE_STATE_CONNECTED;
        } else if (write->handle == watch_link_rx_handles.value_handle) {
            watch_ble_status.received_packets++;
            watch_ble_status.received_bytes += write->len;
            if (write->len >= 4u) {
                watch_ble_status.last_packet_header =
                    ((uint32_t)write->data[0] << 24) |
                    ((uint32_t)write->data[1] << 16) |
                    ((uint32_t)write->data[2] << 8) |
                    write->data[3];
            }
            (void)tseho_link_parser_feed(
                &watch_link_parser,
                write->data,
                write->len,
                watch_link_handle_frame,
                NULL);
        }
    }
}

void watch_ble_poll(void)
{
    uint16_t length;
    uint32_t result;

    do {
        length = sizeof(watch_ble_event);
        result = sd_ble_evt_get(watch_ble_event, &length);
        if (result == NRF_SUCCESS) {
            watch_ble_process_event((const ble_evt_t *)watch_ble_event);
        }
    } while (result == NRF_SUCCESS);

    if ((result != NRF_ERROR_NOT_FOUND) &&
        (watch_ble_status.state != WATCH_BLE_STATE_ERROR)) {
        watch_ble_status.error = (15u << 24) | result;
    }

    if (watch_hello_pending && watch_link_send_hello()) {
        watch_hello_pending = false;
    }
}

const watch_ble_status_t *watch_ble_get_status(void)
{
    return &watch_ble_status;
}

const watch_ble_media_status_t *watch_ble_get_media_status(void)
{
    return &watch_ble_media;
}

bool watch_ble_send_media_command(uint8_t command)
{
    return watch_link_notify(TSEHO_LINK_MSG_MEDIA_COMMAND, 0u,
                             &command, 1u);
}
