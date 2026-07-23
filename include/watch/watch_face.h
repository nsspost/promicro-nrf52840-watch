#ifndef WATCH_WATCH_FACE_H
#define WATCH_WATCH_FACE_H

#include <stdbool.h>
#include <stdint.h>

#include <gno/gno.h>

#include "watch/clock.h"

typedef enum {
    WATCH_SCREEN_HOME_CONTEXT = 0,
    WATCH_SCREEN_DIAGNOSTIC_FACE
} watch_screen_t;

typedef struct {
    gno_context_t *graphics;
    watch_time_t displayed_time;
    bool initialized;
    watch_screen_t screen;
} watch_face_t;

bool watch_face_init(watch_face_t *face, gno_context_t *graphics);
bool watch_face_update(watch_face_t *face, watch_time_t time);
bool watch_face_show_home(watch_face_t *face);
bool watch_face_show_diagnostic(watch_face_t *face);

#endif
