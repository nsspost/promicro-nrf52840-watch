#ifndef WATCH_WATCH_TEXT_H
#define WATCH_WATCH_TEXT_H

#include <stdint.h>
#include <stddef.h>

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

/* Builds a transparent 1-bit glyph mask clipped to the supplied rectangle.
 * The caller owns `bits`, which must contain at least (width * height + 7)/8
 * bytes. It is intended for retained, delta-based text rendering. */
bool watch_text_rasterize_mask(uint8_t *bits, size_t bits_size,
                               uint16_t width, uint16_t height,
                               int x, int y, const char *text,
                               uint8_t scale);
bool watch_text_rasterize_mask_add(uint8_t *bits, size_t bits_size,
                                   uint16_t width, uint16_t height,
                                   int x, int y, const char *text,
                                   uint8_t scale);

#endif
