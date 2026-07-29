#ifndef WATCH_UI_DEMO_MODEL_H
#define WATCH_UI_DEMO_MODEL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    WATCH_UI_QUALITY_GOOD = 0,
    WATCH_UI_QUALITY_UNCERTAIN,
    WATCH_UI_QUALITY_BAD,
    WATCH_UI_QUALITY_STALE,
    WATCH_UI_QUALITY_OFFLINE
} watch_ui_quality_t;

typedef enum {
    WATCH_UI_EVENT_ORDINARY = 0,
    WATCH_UI_EVENT_IMPORTANT,
    WATCH_UI_EVENT_BLOCKING
} watch_ui_event_severity_t;

typedef struct {
    const char *id;
    const char *label;
    const char *compact_label;
    const char *unit;
    int16_t value_tenths;
    watch_ui_quality_t quality;
    uint8_t priority;
} watch_ui_metric_t;

typedef struct {
    const char *source_id;
    const char *location;
    const char *status;
    bool online;
    watch_ui_metric_t metrics[3];
    uint8_t metric_count;
    const char *mode;
} watch_ui_device_t;

typedef struct {
    const char *event_id;
    const char *source_id;
    const char *title;
    const char *detail_line_1;
    const char *detail_line_2;
    watch_ui_event_severity_t severity;
    bool active;
    bool acknowledged;
} watch_ui_event_t;

typedef struct {
    watch_ui_device_t devices[3];
    uint8_t device_count;
    watch_ui_event_t events[3];
    uint8_t event_count;
    uint32_t source_revision;
} watch_ui_demo_model_t;

void watch_ui_demo_model_init(watch_ui_demo_model_t *model);
const char *watch_ui_quality_label(watch_ui_quality_t quality);

#endif
