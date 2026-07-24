/* Generated from the supplied Apache-2.0 Roboto authoring assets. */
#ifndef WATCH_STRICT_CONTEXT_TEXT_H
#define WATCH_STRICT_CONTEXT_TEXT_H

#include <stdint.h>
#include <gno/gno.h>

typedef enum {
    WATCH_STRICT_TEXT_MICRO,
    WATCH_STRICT_TEXT_CAPTION,
    WATCH_STRICT_TEXT_BODY,
    WATCH_STRICT_TEXT_ROW,
    WATCH_STRICT_TEXT_TITLE,
    WATCH_STRICT_TEXT_STATUS,
    WATCH_STRICT_TEXT_DEVICE,
    WATCH_STRICT_TEXT_TIME
} watch_strict_text_style_t;

void watch_strict_draw_text(gno_context_t *graphics, int x, int y,
                            const char *text,
                            watch_strict_text_style_t style,
                            gno_color_t color);
int watch_strict_text_width(const char *text,
                            watch_strict_text_style_t style);
uint8_t watch_strict_text_line_height(watch_strict_text_style_t style);

#endif
