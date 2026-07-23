#include "watch/watch_text.h"

#include <stddef.h>

#define GLYPH(a, b, c, d, e) \
    (uint16_t)(((a) << 12) | ((b) << 9) | ((c) << 6) | ((d) << 3) | (e))

static uint16_t glyph_pattern(char character)
{
    switch (character) {
        case '0': return GLYPH(7, 5, 5, 5, 7);
        case '1': return GLYPH(2, 6, 2, 2, 7);
        case '2': return GLYPH(6, 1, 7, 4, 7);
        case '3': return GLYPH(6, 1, 3, 1, 6);
        case '4': return GLYPH(5, 5, 7, 1, 1);
        case '5': return GLYPH(7, 4, 6, 1, 6);
        case '6': return GLYPH(3, 4, 7, 5, 7);
        case '7': return GLYPH(7, 1, 2, 2, 2);
        case '8': return GLYPH(7, 5, 7, 5, 7);
        case '9': return GLYPH(7, 5, 7, 1, 6);
        case 'A': return GLYPH(2, 5, 7, 5, 5);
        case 'B': return GLYPH(6, 5, 6, 5, 6);
        case 'C': return GLYPH(3, 4, 4, 4, 3);
        case 'D': return GLYPH(6, 5, 5, 5, 6);
        case 'E': return GLYPH(7, 4, 6, 4, 7);
        case 'F': return GLYPH(7, 4, 6, 4, 4);
        case 'G': return GLYPH(3, 4, 5, 5, 3);
        case 'H': return GLYPH(5, 5, 7, 5, 5);
        case 'I': return GLYPH(7, 2, 2, 2, 7);
        case 'J': return GLYPH(1, 1, 1, 5, 2);
        case 'K': return GLYPH(5, 5, 6, 5, 5);
        case 'L': return GLYPH(4, 4, 4, 4, 7);
        case 'M': return GLYPH(5, 7, 7, 5, 5);
        case 'N': return GLYPH(5, 7, 7, 7, 5);
        case 'O': return GLYPH(2, 5, 5, 5, 2);
        case 'P': return GLYPH(6, 5, 6, 4, 4);
        case 'Q': return GLYPH(2, 5, 5, 3, 1);
        case 'R': return GLYPH(6, 5, 6, 5, 5);
        case 'S': return GLYPH(3, 4, 2, 1, 6);
        case 'T': return GLYPH(7, 2, 2, 2, 2);
        case 'U': return GLYPH(5, 5, 5, 5, 7);
        case 'V': return GLYPH(5, 5, 5, 5, 2);
        case 'W': return GLYPH(5, 5, 7, 7, 5);
        case 'X': return GLYPH(5, 5, 2, 5, 5);
        case 'Y': return GLYPH(5, 5, 2, 2, 2);
        case 'Z': return GLYPH(7, 1, 2, 4, 7);
        case '-': return GLYPH(0, 0, 7, 0, 0);
        case ':': return GLYPH(0, 2, 0, 2, 0);
        default: return 0u;
    }
}

void watch_draw_text(gno_context_t *graphics,
                     int x,
                     int y,
                     const char *text,
                     uint8_t scale,
                     gno_color_t color)
{
    if ((graphics == NULL) || (text == NULL) || (scale == 0u)) {
        return;
    }

    int pen_x = x;
    while (*text != '\0') {
        uint16_t pattern = glyph_pattern(*text++);

        for (int row = 0; row < 5; ++row) {
            uint8_t bits =
                (uint8_t)((pattern >> ((4 - row) * 3)) & 0x07u);
            int column = 0;

            while (column < 3) {
                while ((column < 3) &&
                       ((bits & (uint8_t)(4u >> column)) == 0u)) {
                    ++column;
                }
                int run_start = column;
                while ((column < 3) &&
                       ((bits & (uint8_t)(4u >> column)) != 0u)) {
                    ++column;
                }
                if (column > run_start) {
                    gno_fill_rect(graphics,
                                  pen_x + (run_start * scale),
                                  y + (row * scale),
                                  (column - run_start) * scale,
                                  scale,
                                  color);
                }
            }
        }

        pen_x += 4 * scale;
    }
}

int watch_text_width(const char *text, uint8_t scale)
{
    if ((text == NULL) || (scale == 0u)) {
        return 0;
    }

    int characters = 0;
    while (*text++ != '\0') {
        ++characters;
    }
    return (characters > 0) ? ((characters * 4 - 1) * scale) : 0;
}

