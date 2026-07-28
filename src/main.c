#include <stdint.h>

#include "watch/board.h"
#include "watch/ble.h"
#include "watch/clock.h"
#include "watch/cst816.h"
#include "watch/gc9a01.h"
#include "watch/nog_display.h"
#include "watch/watch_text.h"
#include "watch/watch_ui.h"

#define WATCH_DEBUG_MAGIC 0x57415443u /* "WATC" */
#define WATCH_REG32(address) (*(volatile uint32_t *)(uintptr_t)(address))

#define WATCH_CLOCK_BASE              0x40000000u
#define WATCH_CLOCK_TASKS_HFCLKSTART  WATCH_REG32(WATCH_CLOCK_BASE + 0x000u)
#define WATCH_CLOCK_TASKS_HFCLKSTOP   WATCH_REG32(WATCH_CLOCK_BASE + 0x004u)
#define WATCH_CLOCK_EVENTS_HFSTARTED  WATCH_REG32(WATCH_CLOCK_BASE + 0x100u)
#define WATCH_CLOCK_HFCLKSTAT         WATCH_REG32(WATCH_CLOCK_BASE + 0x40Cu)
#define WATCH_HFXO_TIMEOUT_LOOPS      6400000u

typedef struct {
    uint32_t magic;
    uint32_t boot_count;
    uint32_t heartbeat;
    uint32_t last_checkpoint;
    uint32_t touch_present;
    uint32_t touch_chip_id;
    uint32_t touch_project_id;
    uint32_t touch_firmware_version;
    uint32_t touch_gesture;
    uint32_t touch_fingers;
    uint32_t touch_event;
    uint32_t touch_x;
    uint32_t touch_y;
    uint32_t touch_i2c_errors;
    uint32_t clock_hour;
    uint32_t clock_minute;
    uint32_t clock_second;
    uint32_t gui_errors;
    uint32_t gui_state;
    uint32_t clock_subsecond;
    uint32_t command_phase;
    uint32_t critical_phase;
    uint32_t source_revision;
    uint32_t skin;
    uint32_t ble_state;
    uint32_t ble_error;
    uint32_t ble_required_ram_start;
    uint32_t ble_connections;
    uint32_t ble_received_packets;
    uint32_t ble_received_bytes;
    uint32_t ble_last_packet_header;
    uint32_t ble_tx_notifications;
    uint32_t ble_scan_reports;
    int32_t ble_scan_last_rssi;
    uint32_t ble_scan_last_address;
    uint32_t hfxo_started;
    uint32_t hfxo_start_loops;
    uint32_t hfxo_status;
} watch_debug_state_t;

/*
 * Kept across a debugger reset when RAM is preserved. It gives GDB a safe,
 * board-independent way to prove that the application is running.
 */
__attribute__((section(".noinit"), used))
volatile watch_debug_state_t watch_debug_state;

/* Exposed to GDB so the complete test-data UI can be inspected on target. */
__attribute__((used))
watch_ui_t watch_ui_runtime;

/*
 * Non-invasive UI exerciser. J-Link writes x/y and then command:
 * 1 = tap, 2 = guarded hold, 3 = complete pending lifecycle phase,
 * 4 = present event index from x (blocking event 0 proves precedence),
 * 5 = toggle the active built-in skin.
 * The main loop executes it in normal thread context and reports the screen
 * and lifecycle phases in watch_ui_test_result.
 */
__attribute__((used)) volatile uint32_t watch_ui_test_command;
__attribute__((used)) volatile uint32_t watch_ui_test_x;
__attribute__((used)) volatile uint32_t watch_ui_test_y;
__attribute__((used)) volatile uint32_t watch_ui_test_result;

static bool watch_hfxo_probe(uint32_t *loops_used, uint32_t *status)
{
    uint32_t loops = 0u;

    WATCH_CLOCK_EVENTS_HFSTARTED = 0u;
    WATCH_CLOCK_TASKS_HFCLKSTART = 1u;
    while ((WATCH_CLOCK_EVENTS_HFSTARTED == 0u) &&
           (loops < WATCH_HFXO_TIMEOUT_LOOPS)) {
        ++loops;
    }

    *loops_used = loops;
    *status = WATCH_CLOCK_HFCLKSTAT;
    bool started = (WATCH_CLOCK_EVENTS_HFSTARTED != 0u) &&
                   ((*status & 1u) != 0u);

    WATCH_CLOCK_TASKS_HFCLKSTOP = 1u;
    return started;
}

static void watch_show_boot_diagnostic(gno_context_t *graphics,
                                       const char *line1,
                                       const char *line2,
                                       gno_color_t color)
{
    gno_fill_rect(graphics, 24, 72, 192, 84, GNO_RGB(0, 0, 0));
    gno_draw_rect(graphics, 24, 72, 192, 84, color);
    watch_draw_text(graphics, 42, 91, line1, 2u, color);
    watch_draw_text(graphics, 42, 126, line2, 1u,
                    GNO_RGB(220, 225, 230));
}

static watch_ui_phone_state_t watch_phone_state_from_ble(uint32_t ble_state)
{
    switch (ble_state) {
        case WATCH_BLE_STATE_ADVERTISING:
            return WATCH_UI_PHONE_ADVERTISING;
        case WATCH_BLE_STATE_CONNECTED:
            return WATCH_UI_PHONE_CONNECTED;
        case WATCH_BLE_STATE_SUBSCRIBED:
            return WATCH_UI_PHONE_SUBSCRIBED;
        case WATCH_BLE_STATE_READY:
            return WATCH_UI_PHONE_READY;
        case WATCH_BLE_STATE_ERROR:
            return WATCH_UI_PHONE_ERROR;
        case WATCH_BLE_STATE_OFF:
        default:
            return WATCH_UI_PHONE_OFF;
    }
}

int main(void)
{
    if (watch_debug_state.magic != WATCH_DEBUG_MAGIC) {
        watch_debug_state.magic = WATCH_DEBUG_MAGIC;
        watch_debug_state.boot_count = 0;
    }

    watch_debug_state.boot_count++;
    watch_debug_state.heartbeat = 0;
    watch_debug_state.last_checkpoint = 1;
    watch_debug_state.touch_present = 0;
    watch_debug_state.touch_chip_id = 0;
    watch_debug_state.touch_project_id = 0;
    watch_debug_state.touch_firmware_version = 0;
    watch_debug_state.touch_gesture = 0;
    watch_debug_state.touch_fingers = 0;
    watch_debug_state.touch_event = 0;
    watch_debug_state.touch_x = 0;
    watch_debug_state.touch_y = 0;
    watch_debug_state.touch_i2c_errors = 0;
    watch_debug_state.clock_hour = 0;
    watch_debug_state.clock_minute = 0;
    watch_debug_state.clock_second = 0;
    watch_debug_state.gui_errors = 0;
    watch_debug_state.gui_state = 0;
    watch_debug_state.clock_subsecond = 0;
    watch_debug_state.command_phase = 0;
    watch_debug_state.critical_phase = 0;
    watch_debug_state.source_revision = 0;
    watch_debug_state.skin = 0;
    watch_debug_state.ble_state = WATCH_BLE_STATE_OFF;
    watch_debug_state.ble_error = 0;
    watch_debug_state.ble_required_ram_start = 0;
    watch_debug_state.ble_connections = 0;
    watch_debug_state.ble_received_packets = 0;
    watch_debug_state.ble_received_bytes = 0;
    watch_debug_state.ble_last_packet_header = 0;
    watch_debug_state.ble_tx_notifications = 0;
    watch_debug_state.ble_scan_reports = 0;
    watch_debug_state.ble_scan_last_rssi = 0;
    watch_debug_state.ble_scan_last_address = 0;
    watch_debug_state.hfxo_started = 0;
    watch_debug_state.hfxo_start_loops = 0;
    watch_debug_state.hfxo_status = 0;
    watch_ui_test_command = 0u;
    watch_ui_test_x = 0u;
    watch_ui_test_y = 0u;
    watch_ui_test_result = 0u;

    watch_board_init();
    watch_debug_state.last_checkpoint = 10;
    watch_debug_state.hfxo_started =
        watch_hfxo_probe(
            (uint32_t *)&watch_debug_state.hfxo_start_loops,
            (uint32_t *)&watch_debug_state.hfxo_status) ? 1u : 0u;

    if (!gc9a01_init()) {
        watch_debug_state.last_checkpoint = 0xE001u;
        for (;;) {
            watch_debug_state.heartbeat++;
            watch_delay_ms(250u);
        }
    }

    gno_context_t graphics;
    if (!watch_nog_display_init(&graphics)) {
        watch_debug_state.last_checkpoint = 0xE002u;
        for (;;) {
            watch_debug_state.heartbeat++;
            watch_delay_ms(250u);
        }
    }

    if (!watch_ui_init(&watch_ui_runtime, &graphics)) {
        watch_debug_state.last_checkpoint = 0xE003u;
    }

    if (watch_debug_state.hfxo_started == 0u) {
        watch_debug_state.last_checkpoint = 0xE009u;
        watch_show_boot_diagnostic(&graphics, "HFXO FAIL",
                                   "32 MHZ CRYSTAL", GNO_RGB(255, 60, 40));
        for (;;) {
            watch_status_led_set(true);
            watch_delay_ms(100u);
            watch_status_led_set(false);
            watch_delay_ms(100u);
            watch_debug_state.heartbeat++;
        }
    }

    if (!watch_ble_init()) {
        const watch_ble_status_t *ble = watch_ble_get_status();
        watch_debug_state.ble_state = ble->state;
        watch_debug_state.ble_error = ble->error;
        watch_debug_state.ble_required_ram_start =
            ble->required_ram_start;
        watch_debug_state.last_checkpoint = 0xE008u;
        watch_show_boot_diagnostic(&graphics, "BLE FAIL",
                                   "SOFTDEVICE INIT", GNO_RGB(255, 150, 20));
        for (;;) {
            watch_status_led_set(true);
            watch_delay_ms(500u);
            watch_status_led_set(false);
            watch_delay_ms(500u);
            watch_debug_state.heartbeat++;
        }
    }

    watch_status_led_set(true);
    watch_show_boot_diagnostic(&graphics, "HFXO OK",
                               "BLE ADVERTISING", GNO_RGB(30, 220, 120));
    watch_delay_ms(1500u);

    watch_clock_init(WATCH_INITIAL_HOUR,
                     WATCH_INITIAL_MINUTE,
                     WATCH_INITIAL_SECOND);
    watch_time_t time = watch_clock_get();

    cst816_identity_t identity;
    if (cst816_init(&identity)) {
        watch_debug_state.touch_present = 1u;
        watch_debug_state.touch_chip_id = identity.chip_id;
        watch_debug_state.touch_project_id = identity.project_id;
        watch_debug_state.touch_firmware_version = identity.firmware_version;
        watch_debug_state.last_checkpoint = 40u;
    } else {
        watch_debug_state.last_checkpoint = 0xE004u;
    }

    for (;;) {
        watch_ble_poll();
        const watch_ble_status_t *ble = watch_ble_get_status();
        watch_debug_state.ble_state = ble->state;
        watch_debug_state.ble_error = ble->error;
        watch_debug_state.ble_required_ram_start =
            ble->required_ram_start;
        watch_debug_state.ble_connections = ble->connections;
        watch_debug_state.ble_received_packets =
            ble->received_packets;
        watch_debug_state.ble_received_bytes = ble->received_bytes;
        watch_debug_state.ble_last_packet_header =
            ble->last_packet_header;
        watch_debug_state.ble_tx_notifications =
            ble->tx_notifications;
        watch_debug_state.ble_scan_reports = ble->scan_reports;
        watch_debug_state.ble_scan_last_rssi = ble->scan_last_rssi;
        watch_debug_state.ble_scan_last_address =
            ble->scan_last_address;
        (void)watch_ui_set_phone_status(
            &watch_ui_runtime,
            (watch_ui_phone_status_t) {
                .state = watch_phone_state_from_ble(ble->state),
                .connections = ble->connections,
                .received_packets = ble->received_packets,
                .tx_notifications = ble->tx_notifications
            });
        const watch_ble_media_status_t *ble_media =
            watch_ble_get_media_status();
        watch_ui_media_status_t media = {
            .state = ble_media->state,
            .volume = ble_media->volume,
            .duration_s = ble_media->duration_s,
            .position_s = ble_media->position_s,
            .revision = ble_media->revision
        };
        for (uint16_t i = 0u; i < sizeof(media.artist); ++i) {
            media.artist[i] = ble_media->artist[i];
            if (ble_media->artist[i] == '\0') break;
        }
        for (uint16_t i = 0u; i < sizeof(media.track); ++i) {
            media.track[i] = ble_media->track[i];
            if (ble_media->track[i] == '\0') break;
        }
        (void)watch_ui_set_media_status(&watch_ui_runtime, &media);
        watch_debug_state.heartbeat++;
        time = watch_clock_get();
        watch_debug_state.clock_hour = time.hour;
        watch_debug_state.clock_minute = time.minute;
        watch_debug_state.clock_second = time.second;
        watch_debug_state.clock_subsecond = time.subsecond;

        if (!watch_ui_update(&watch_ui_runtime, time)) {
            watch_debug_state.gui_errors++;
            watch_debug_state.last_checkpoint = 0xE006u;
        }
        watch_debug_state.gui_state =
            (uint32_t)watch_ui_get_screen(&watch_ui_runtime);
        watch_debug_state.command_phase =
            (uint32_t)watch_ui_runtime.command_phase;
        watch_debug_state.critical_phase =
            (uint32_t)watch_ui_runtime.critical_phase;
        watch_debug_state.source_revision =
            watch_ui_runtime.model.source_revision;
        watch_debug_state.skin =
            (uint32_t)watch_ui_get_skin(&watch_ui_runtime);

        uint32_t test_command = watch_ui_test_command;
        if (test_command != 0u) {
            uint16_t test_x = (uint16_t)watch_ui_test_x;
            uint16_t test_y = (uint16_t)watch_ui_test_y;
            watch_ui_test_command = 0u;
            bool test_ok = false;
            if (test_command == 1u) {
                test_ok = watch_ui_debug_tap(&watch_ui_runtime,
                                             test_x, test_y);
            } else if (test_command == 2u) {
                test_ok = watch_ui_debug_hold(&watch_ui_runtime,
                                              test_x, test_y);
            } else if (test_command == 3u) {
                test_ok =
                    watch_ui_debug_complete_phase(&watch_ui_runtime);
            } else if (test_command == 4u) {
                test_ok = watch_ui_present_event(
                    &watch_ui_runtime, (uint8_t)test_x);
            } else if (test_command == 5u) {
                watch_ui_skin_t next_skin =
                    (watch_ui_get_skin(&watch_ui_runtime) ==
                     WATCH_UI_SKIN_STRICT_CONTEXT) ?
                    WATCH_UI_SKIN_COLOR_CONTEXT :
                    WATCH_UI_SKIN_STRICT_CONTEXT;
                test_ok = watch_ui_set_skin(&watch_ui_runtime, next_skin);
            }
            uint32_t command_phase =
                (watch_ui_runtime.screen ==
                 WATCH_UI_SCREEN_COMMAND_STATUS) ?
                (uint32_t)watch_ui_runtime.command_phase : 0u;
            uint32_t critical_phase =
                (watch_ui_runtime.screen ==
                 WATCH_UI_SCREEN_CRITICAL_REQUEST) ?
                (uint32_t)watch_ui_runtime.critical_phase : 0u;
            watch_ui_test_result =
                (test_ok ? 0x600D0000u : 0xBAD00000u) |
                (critical_phase << 12) |
                (command_phase << 8) |
                ((uint32_t)watch_ui_get_skin(&watch_ui_runtime) << 7) |
                (uint32_t)watch_ui_runtime.screen;
        }

        if (watch_debug_state.touch_present != 0u) {
            cst816_touch_t touch;
            if (cst816_read_touch(&touch)) {
                watch_debug_state.touch_gesture = touch.gesture;
                watch_debug_state.touch_fingers = touch.fingers;
                watch_debug_state.touch_event = touch.event;
                watch_debug_state.touch_x = touch.x;
                watch_debug_state.touch_y = touch.y;

                if (!watch_ui_process_touch(&watch_ui_runtime,
                                            touch.fingers != 0u,
                                            touch.x,
                                            touch.y,
                                            time)) {
                    watch_debug_state.gui_errors++;
                    watch_debug_state.last_checkpoint = 0xE007u;
                } else if (touch.fingers != 0u) {
                    watch_debug_state.last_checkpoint = 41u;
                } else {
                    watch_debug_state.last_checkpoint = 40u;
                }
            } else {
                watch_debug_state.touch_i2c_errors++;
                watch_debug_state.last_checkpoint = 0xE005u;
            }
        }

        uint8_t media_command;
        if (watch_ui_take_media_command(&watch_ui_runtime, &media_command)) {
            if (!watch_ble_send_media_command(media_command)) {
                watch_debug_state.ble_error = 0x4D454449u; /* MEDI */
            }
        }

        watch_delay_ms(20u);
    }
}
