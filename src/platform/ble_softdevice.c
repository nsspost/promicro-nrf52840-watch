#include "watch/ble.h"

#include <stddef.h>
#include <stdint.h>

#include "ble.h"
#include "ble_gap.h"
#include "ble_gatt.h"
#include "ble_gatts.h"
#include "nrf_error.h"
#include "nrf_sdm.h"

#define WATCH_BLE_RAM_START       0x20004000u
#define WATCH_BLE_EVENT_BUFFER    256u
#define WATCH_BLE_CONN_TAG        1u
#define WATCH_BLE_VALUE_MAX_LEN   20u
#define WATCH_BLE_SCAN_BUFFER     64u

#define WATCH_NUS_SERVICE_UUID    0x0001u
#define WATCH_NUS_RX_UUID         0x0002u
#define WATCH_NUS_TX_UUID         0x0003u

static const uint8_t watch_device_name[] = "Tseho Watch";
static const ble_uuid128_t watch_nus_base_uuid = {
    .uuid128 = {
        0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
        0x93, 0xF3, 0xA3, 0xB5, 0x00, 0x00, 0x40, 0x6E
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
    0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
    0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E
};

static __attribute__((aligned(4))) uint8_t watch_ble_event[WATCH_BLE_EVENT_BUFFER];
static __attribute__((aligned(4))) uint8_t watch_ble_scan_data[WATCH_BLE_SCAN_BUFFER];
static ble_data_t watch_ble_scan_buffer = {
    .p_data = watch_ble_scan_data,
    .len = sizeof(watch_ble_scan_data)
};
static watch_ble_status_t watch_ble_status;
static ble_gatts_char_handles_t watch_nus_rx_handles;
static ble_gatts_char_handles_t watch_nus_tx_handles;
static uint16_t watch_conn_handle = BLE_CONN_HANDLE_INVALID;
static uint8_t watch_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static uint8_t watch_nus_uuid_type;
static bool watch_chronos_request_pending;

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

static bool watch_nus_add_characteristic(uint16_t service_handle,
                                         uint16_t uuid_value,
                                         bool rx,
                                         ble_gatts_char_handles_t *handles)
{
    ble_uuid_t uuid = {
        .uuid = uuid_value,
        .type = watch_nus_uuid_type
    };
    ble_gatts_attr_md_t value_md = {0};
    ble_gatts_attr_t value_attr = {0};
    ble_gatts_char_md_t char_md = {0};
    ble_gatts_attr_md_t cccd_md = {0};

    watch_open_security(&value_md.read_perm);
    watch_open_security(&value_md.write_perm);
    value_md.vloc = BLE_GATTS_VLOC_STACK;
    value_md.vlen = 1u;

    value_attr.p_uuid = &uuid;
    value_attr.p_attr_md = &value_md;
    value_attr.init_len = 0u;
    value_attr.max_len = WATCH_BLE_VALUE_MAX_LEN;

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

static bool watch_nus_init(void)
{
    ble_uuid_t service_uuid = {0};
    uint16_t service_handle;

    if (!watch_ble_check(sd_ble_uuid_vs_add(&watch_nus_base_uuid,
                                            &watch_nus_uuid_type), 5u)) {
        return false;
    }
    service_uuid.type = watch_nus_uuid_type;
    service_uuid.uuid = WATCH_NUS_SERVICE_UUID;
    if (!watch_ble_check(
            sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                     &service_uuid,
                                     &service_handle),
            6u)) {
        return false;
    }
    if (!watch_nus_add_characteristic(service_handle,
                                      WATCH_NUS_RX_UUID, true,
                                      &watch_nus_rx_handles)) {
        return false;
    }
    return watch_nus_add_characteristic(service_handle,
                                        WATCH_NUS_TX_UUID, false,
                                        &watch_nus_tx_handles);
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

static bool watch_scanning_start(bool configure)
{
    ble_gap_scan_params_t params = {
        .active = 0u,
        .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
        .scan_phys = BLE_GAP_PHY_1MBPS,
        .interval = 160u,
        .window = 80u,
        .timeout = 0u
    };

    watch_ble_scan_buffer.len = sizeof(watch_ble_scan_data);
    return watch_ble_check(
        sd_ble_gap_scan_start(configure ? &params : NULL,
                              &watch_ble_scan_buffer),
        17u);
}

static void watch_chronos_sync_request(void)
{
    static const uint8_t packet[] = {
        0xABu, 0x00u, 0x03u, 0xFEu, 0x23u, 0x80u
    };
    uint16_t length = sizeof(packet);
    ble_gatts_hvx_params_t params = {
        .handle = watch_nus_tx_handles.value_handle,
        .type = BLE_GATT_HVX_NOTIFICATION,
        .offset = 0u,
        .p_len = &length,
        .p_data = packet
    };
    uint32_t result = sd_ble_gatts_hvx(watch_conn_handle, &params);
    if (result == NRF_SUCCESS) {
        watch_ble_status.chronos_sync_requests++;
        watch_chronos_request_pending = false;
        watch_ble_status.state = WATCH_BLE_STATE_CHRONOS_READY;
    } else if (result != NRF_ERROR_RESOURCES) {
        watch_ble_status.error = (14u << 24) | result;
    }
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
    watch_chronos_request_pending = false;

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
    cfg.gap_cfg.role_count_cfg.central_role_count = 1u;
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
    if (!watch_nus_init()) {
        return false;
    }
    if (!watch_advertising_start(true)) {
        return false;
    }
    return watch_scanning_start(true);
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
        watch_chronos_request_pending = false;
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
        if ((write->handle == watch_nus_tx_handles.cccd_handle) &&
            (write->len == 2u)) {
            watch_chronos_request_pending = (write->data[0] & 1u) != 0u;
            if (!watch_chronos_request_pending) {
                watch_ble_status.state = WATCH_BLE_STATE_CONNECTED;
            }
        } else if (write->handle == watch_nus_rx_handles.value_handle) {
            watch_ble_status.received_packets++;
            watch_ble_status.received_bytes += write->len;
            if (write->len >= 4u) {
                watch_ble_status.last_packet_header =
                    ((uint32_t)write->data[0] << 24) |
                    ((uint32_t)write->data[1] << 16) |
                    ((uint32_t)write->data[2] << 8) |
                    write->data[3];
            }
        }
    } else if (event_id == BLE_GAP_EVT_ADV_REPORT) {
        const ble_gap_evt_adv_report_t *report =
            &event->evt.gap_evt.params.adv_report;
        watch_ble_status.scan_reports++;
        watch_ble_status.scan_last_rssi = report->rssi;
        watch_ble_status.scan_last_address =
            ((uint32_t)report->peer_addr.addr[3] << 24) |
            ((uint32_t)report->peer_addr.addr[2] << 16) |
            ((uint32_t)report->peer_addr.addr[1] << 8) |
            report->peer_addr.addr[0];
        (void)watch_scanning_start(false);
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
    if (watch_chronos_request_pending) {
        watch_chronos_sync_request();
    }
}

const watch_ble_status_t *watch_ble_get_status(void)
{
    return &watch_ble_status;
}
