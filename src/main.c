#include <stdint.h>

#include "GuiSm.h"
#include "watch/board.h"
#include "watch/clock.h"
#include "watch/cst816.h"
#include "watch/gc9a01.h"
#include "watch/gui_state_actions.h"
#include "watch/nog_display.h"
#include "watch/watch_face.h"

#define WATCH_DEBUG_MAGIC 0x57415443u /* "WATC" */

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
} watch_debug_state_t;

/*
 * Kept across a debugger reset when RAM is preserved. It gives GDB a safe,
 * board-independent way to prove that the application is running.
 */
__attribute__((section(".noinit"), used))
volatile watch_debug_state_t watch_debug_state;

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

    watch_board_init();
    watch_debug_state.last_checkpoint = 10;

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

    watch_face_t face;
    if (!watch_face_init(&face, &graphics)) {
        watch_debug_state.last_checkpoint = 0xE003u;
    }

    watch_clock_init(WATCH_INITIAL_HOUR,
                     WATCH_INITIAL_MINUTE,
                     WATCH_INITIAL_SECOND);
    watch_gui_bind_face(&face);
    GuiSm gui_sm;
    GuiSm_ctor(&gui_sm);
    GuiSm_start(&gui_sm);
    watch_debug_state.gui_state = (uint32_t)gui_sm.state_id;
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

    bool touch_was_down = false;

    for (;;) {
        watch_debug_state.heartbeat++;
        time = watch_clock_get();
        watch_debug_state.clock_hour = time.hour;
        watch_debug_state.clock_minute = time.minute;
        watch_debug_state.clock_second = time.second;
        watch_debug_state.clock_subsecond = time.subsecond;

        if (!watch_face_update(&face, time)) {
            watch_debug_state.gui_errors++;
            watch_debug_state.last_checkpoint = 0xE006u;
        }

        if (watch_debug_state.touch_present != 0u) {
            cst816_touch_t touch;
            if (cst816_read_touch(&touch)) {
                watch_debug_state.touch_gesture = touch.gesture;
                watch_debug_state.touch_fingers = touch.fingers;
                watch_debug_state.touch_event = touch.event;
                watch_debug_state.touch_x = touch.x;
                watch_debug_state.touch_y = touch.y;

                bool touch_is_down = touch.fingers != 0u;
                if (touch_is_down && !touch_was_down) {
                    GuiSm_dispatch_event(&gui_sm, GuiSm_EventId_ACTIVATE);
                    watch_debug_state.gui_state = (uint32_t)gui_sm.state_id;
                    watch_debug_state.last_checkpoint = 41u;
                } else {
                    watch_debug_state.last_checkpoint = 40u;
                }
                touch_was_down = touch_is_down;
            } else {
                watch_debug_state.touch_i2c_errors++;
                watch_debug_state.last_checkpoint = 0xE005u;
            }
        }

        watch_delay_ms(20u);
    }
}
