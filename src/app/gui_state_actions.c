#include "watch/gui_state_actions.h"

#include "watch/clock.h"

static watch_face_t *bound_face;

void watch_gui_bind_face(watch_face_t *face)
{
    bound_face = face;
}

static void show_screen(bool home)
{
    if (bound_face == 0) {
        return;
    }

    if (home) {
        (void)watch_face_show_home(bound_face);
    } else {
        (void)watch_face_show_diagnostic(bound_face);
    }
    (void)watch_face_update(bound_face, watch_clock_get());
}

void watch_gui_show_home(void)
{
    show_screen(true);
}

void watch_gui_show_diagnostic(void)
{
    show_screen(false);
}
