#ifndef WATCH_STRICT_CONTEXT_ICONS_H
#define WATCH_STRICT_CONTEXT_ICONS_H

#include <gno/gno.h>

typedef enum {
    WATCH_STRICT_ICON_BATTERY,
    WATCH_STRICT_ICON_CLOUD,
    WATCH_STRICT_ICON_CARET_RIGHT,
    WATCH_STRICT_ICON_CHAT,
    WATCH_STRICT_ICON_ECOSYSTEM,
    WATCH_STRICT_ICON_CARET_LEFT,
    WATCH_STRICT_ICON_LIST,
    WATCH_STRICT_ICON_SLIDERS,
    WATCH_STRICT_ICON_LOCK,
    WATCH_STRICT_ICON_STOP,
    WATCH_STRICT_ICON_WARNING,
    WATCH_STRICT_ICON_CHECK,
    WATCH_STRICT_ICON_ERROR,
    WATCH_STRICT_ICON_SPINNER
} watch_strict_icon_t;

void watch_strict_draw_icon(gno_context_t *graphics,
                            int x,
                            int y,
                            watch_strict_icon_t icon,
                            gno_color_t color);

#endif
