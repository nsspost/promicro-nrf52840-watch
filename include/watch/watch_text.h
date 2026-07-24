#ifndef WATCH_WATCH_TEXT_H
#define WATCH_WATCH_TEXT_H

#include <stdint.h>

#include <gno/gno.h>

/*
 * Compact watch-skin font. Uppercase ASCII, uppercase Russian UTF-8 and
 * digits are rendered from a bounded 3x5 pattern without dynamic allocation.
 */
void watch_draw_text(gno_context_t *graphics,
                     int x,
                     int y,
                     const char *text,
                     uint8_t scale,
                     gno_color_t color);

int watch_text_width(const char *text, uint8_t scale);

#endif
