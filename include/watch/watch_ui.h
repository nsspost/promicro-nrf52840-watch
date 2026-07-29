#ifndef WATCH_WATCH_UI_H
#define WATCH_WATCH_UI_H

#include <stdbool.h>
#include <stdint.h>

#include <gno/gno.h>

#include "watch/clock.h"
#include "watch/ui_demo_model.h"

enum {
    WATCH_UI_NAV_DEPTH = 4,
    WATCH_UI_MAX_HIT_TARGETS = 8
};

typedef enum {
    WATCH_UI_SCREEN_HOME_CONTEXT = 0,
    WATCH_UI_SCREEN_DEVICE_LIST,
    WATCH_UI_SCREEN_DEVICE_OVERVIEW,
    WATCH_UI_SCREEN_PARAMETER_LIST,
    WATCH_UI_SCREEN_METRIC_DETAIL,
    WATCH_UI_SCREEN_CONTROL_LIST,
    WATCH_UI_SCREEN_COMMAND_STATUS,
    WATCH_UI_SCREEN_CRITICAL_REQUEST,
    WATCH_UI_SCREEN_EVENT_JOURNAL,
    WATCH_UI_SCREEN_EVENT_DETAIL,
    WATCH_UI_SCREEN_DIAGNOSTIC,
    WATCH_UI_SCREEN_MEDIA
} watch_ui_screen_t;

typedef enum {
    WATCH_UI_COMMAND_IDLE = 0,
    WATCH_UI_COMMAND_CONFIRM,
    WATCH_UI_COMMAND_SENDING,
    WATCH_UI_COMMAND_SUCCESS,
    WATCH_UI_COMMAND_REJECTED,
    WATCH_UI_COMMAND_TIMEOUT
} watch_ui_command_phase_t;

typedef enum {
    WATCH_UI_CRITICAL_IDLE = 0,
    WATCH_UI_CRITICAL_CONFIRM,
    WATCH_UI_CRITICAL_WAITING,
    WATCH_UI_CRITICAL_APPROVED,
    WATCH_UI_CRITICAL_DENIED
} watch_ui_critical_phase_t;

typedef enum {
    WATCH_UI_SKIN_COLOR_CONTEXT = 0,
    WATCH_UI_SKIN_STRICT_CONTEXT
} watch_ui_skin_t;

typedef enum {
    WATCH_UI_PHONE_OFF = 0,
    WATCH_UI_PHONE_ADVERTISING,
    WATCH_UI_PHONE_CONNECTED,
    WATCH_UI_PHONE_SUBSCRIBED,
    WATCH_UI_PHONE_READY,
    WATCH_UI_PHONE_ERROR
} watch_ui_phone_state_t;

typedef struct {
    watch_ui_phone_state_t state;
    uint32_t connections;
    uint32_t received_packets;
    uint32_t tx_notifications;
} watch_ui_phone_status_t;

typedef struct {
    uint8_t state;
    uint8_t volume;
    uint32_t duration_s;
    uint32_t position_s;
    char artist[49];
    char track[97];
    uint32_t revision;
} watch_ui_media_status_t;

typedef struct {
    const uint8_t *pixels;
    uint16_t width;
    uint16_t height;
    uint32_t revision;
    bool valid;
} watch_ui_artwork_status_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    uint8_t action;
    uint8_t argument;
    bool requires_hold;
} watch_ui_hit_target_t;

typedef struct {
    gno_context_t *graphics;
    watch_ui_demo_model_t model;
    watch_ui_screen_t screen;
    watch_ui_screen_t navigation[WATCH_UI_NAV_DEPTH];
    uint8_t navigation_depth;
    uint8_t selected_device;
    uint8_t selected_metric;
    uint8_t selected_event;
    watch_ui_hit_target_t hits[WATCH_UI_MAX_HIT_TARGETS];
    uint8_t hit_count;
    int8_t pressed_hit;
    bool touch_down;
    bool hold_active;
    uint32_t hold_started_at;
    uint8_t hold_progress;
    watch_ui_command_phase_t command_phase;
    watch_ui_critical_phase_t critical_phase;
    watch_ui_skin_t skin;
    uint8_t command_demo_outcome;
    uint8_t critical_demo_outcome;
    watch_ui_phone_status_t phone;
    watch_ui_media_status_t media;
    watch_ui_artwork_status_t artwork;
    uint8_t media_command;
    bool media_command_pending;
    uint32_t phase_started_at;
    watch_time_t displayed_time;
    bool initialized;
    bool needs_redraw;
} watch_ui_t;

bool watch_ui_init(watch_ui_t *ui, gno_context_t *graphics);
bool watch_ui_update(watch_ui_t *ui, watch_time_t time);
bool watch_ui_process_touch(watch_ui_t *ui,
                            bool down,
                            uint16_t x,
                            uint16_t y,
                            watch_time_t time);
bool watch_ui_present_event(watch_ui_t *ui, uint8_t event_index);
watch_ui_screen_t watch_ui_get_screen(const watch_ui_t *ui);
watch_ui_skin_t watch_ui_get_skin(const watch_ui_t *ui);
bool watch_ui_set_skin(watch_ui_t *ui, watch_ui_skin_t skin);
bool watch_ui_set_phone_status(watch_ui_t *ui,
                               watch_ui_phone_status_t status);
bool watch_ui_set_media_status(watch_ui_t *ui,
                               const watch_ui_media_status_t *status);
bool watch_ui_set_artwork_status(watch_ui_t *ui,
                                 watch_ui_artwork_status_t status);
bool watch_ui_take_media_command(watch_ui_t *ui, uint8_t *command);

/*
 * Deterministic helpers for GDB and manufacturing checks. They exercise the
 * same hit targets and lifecycle transitions as the physical touch adapter.
 */
bool watch_ui_debug_tap(watch_ui_t *ui, uint16_t x, uint16_t y);
bool watch_ui_debug_hold(watch_ui_t *ui, uint16_t x, uint16_t y);
bool watch_ui_debug_complete_phase(watch_ui_t *ui);
bool watch_ui_debug_validate(const watch_ui_t *ui);

#endif
