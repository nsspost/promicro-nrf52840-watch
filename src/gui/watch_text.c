#include "watch/watch_text.h"

#include <stddef.h>

static void mask_set(uint8_t *bits, uint32_t index)
{
    bits[index >> 3] |= (uint8_t)(1u << (index & 7u));
}

#define GLYPH(a, b, c, d, e) \
    (uint16_t)(((a) << 12) | ((b) << 9) | ((c) << 6) | ((d) << 3) | (e))

static uint32_t next_codepoint(const char **text)
{
    const uint8_t *input = (const uint8_t *)*text;
    uint32_t codepoint;

    if (input[0] < 0x80u) {
        codepoint = input[0];
        *text += 1;
    } else if ((input[0] & 0xE0u) == 0xC0u &&
               (input[1] & 0xC0u) == 0x80u) {
        codepoint = ((uint32_t)(input[0] & 0x1Fu) << 6) |
                    (uint32_t)(input[1] & 0x3Fu);
        *text += 2;
    } else {
        codepoint = '?';
        *text += 1;
    }
    return codepoint;
}

static uint16_t glyph_pattern(uint32_t character)
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
        case 'a': return GLYPH(2, 5, 7, 5, 5);
        case 'b': return GLYPH(6, 5, 6, 5, 6);
        case 'r': return GLYPH(6, 5, 6, 5, 5);
        case '-': return GLYPH(0, 0, 7, 0, 0);
        case ':': return GLYPH(0, 2, 0, 2, 0);
        case '.': return GLYPH(0, 0, 0, 0, 2);
        case '%': return GLYPH(5, 1, 2, 4, 5);
        case '/': return GLYPH(1, 1, 2, 4, 4);
        case '+': return GLYPH(0, 2, 7, 2, 0);
        case '!': return GLYPH(2, 2, 2, 0, 2);
        case '?': return GLYPH(6, 1, 2, 0, 2);
        case '>': return GLYPH(4, 2, 1, 2, 4);
        case '<': return GLYPH(1, 2, 4, 2, 1);
        case 0x0401u: return GLYPH(5, 7, 4, 6, 7); /* Ё */
        case 0x0410u: return GLYPH(2, 5, 7, 5, 5); /* А */
        case 0x0411u: return GLYPH(7, 4, 6, 5, 6); /* Б */
        case 0x0412u: return GLYPH(6, 5, 6, 5, 6); /* В */
        case 0x0413u: return GLYPH(7, 4, 4, 4, 4); /* Г */
        case 0x0414u: return GLYPH(2, 5, 5, 7, 5); /* Д */
        case 0x0415u: return GLYPH(7, 4, 6, 4, 7); /* Е */
        case 0x0416u: return GLYPH(5, 2, 7, 2, 5); /* Ж */
        case 0x0417u: return GLYPH(6, 1, 3, 1, 6); /* З */
        case 0x0418u: return GLYPH(5, 5, 7, 7, 5); /* И */
        case 0x0419u: return GLYPH(2, 5, 7, 7, 5); /* Й */
        case 0x041Au: return GLYPH(5, 5, 6, 5, 5); /* К */
        case 0x041Bu: return GLYPH(3, 5, 5, 5, 5); /* Л */
        case 0x041Cu: return GLYPH(5, 7, 7, 5, 5); /* М */
        case 0x041Du: return GLYPH(5, 5, 7, 5, 5); /* Н */
        case 0x041Eu: return GLYPH(2, 5, 5, 5, 2); /* О */
        case 0x041Fu: return GLYPH(7, 5, 5, 5, 5); /* П */
        case 0x0420u: return GLYPH(6, 5, 6, 4, 4); /* Р */
        case 0x0421u: return GLYPH(3, 4, 4, 4, 3); /* С */
        case 0x0422u: return GLYPH(7, 2, 2, 2, 2); /* Т */
        case 0x0423u: return GLYPH(5, 5, 3, 1, 6); /* У */
        case 0x0424u: return GLYPH(2, 7, 2, 7, 2); /* Ф */
        case 0x0425u: return GLYPH(5, 5, 2, 5, 5); /* Х */
        case 0x0426u: return GLYPH(5, 5, 5, 7, 1); /* Ц */
        case 0x0427u: return GLYPH(5, 5, 3, 1, 1); /* Ч */
        case 0x0428u: return GLYPH(5, 5, 5, 5, 7); /* Ш */
        case 0x0429u: return GLYPH(5, 5, 5, 7, 1); /* Щ */
        case 0x042Au: return GLYPH(4, 4, 6, 5, 6); /* Ъ */
        case 0x042Bu: return GLYPH(5, 5, 7, 5, 7); /* Ы */
        case 0x042Cu: return GLYPH(4, 4, 6, 5, 6); /* Ь */
        case 0x042Du: return GLYPH(6, 1, 3, 1, 6); /* Э */
        case 0x042Eu: return GLYPH(5, 7, 7, 7, 5); /* Ю */
        case 0x042Fu: return GLYPH(3, 5, 3, 5, 5); /* Я */
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
        uint16_t pattern = glyph_pattern(next_codepoint(&text));

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
    while (*text != '\0') {
        (void)next_codepoint(&text);
        ++characters;
    }
    return (characters > 0) ? ((characters * 4 - 1) * scale) : 0;
}

static bool watch_text_rasterize_mask_impl(uint8_t *bits, size_t bits_size,
                                            uint16_t width, uint16_t height,
                                            int x, int y, const char *text,
                                            uint8_t scale, bool clear)
{
    size_t required = ((size_t)width * height + 7u) / 8u;
    if ((bits == NULL) || (text == NULL) || (scale == 0u) ||
        (bits_size < required)) return false;
    if (clear) {
        for (size_t i = 0u; i < required; ++i) bits[i] = 0u;
    }

    int pen_x = x;
    while (*text != '\0') {
        uint16_t pattern = glyph_pattern(next_codepoint(&text));
        for (int row = 0; row < 5; ++row) {
            uint8_t row_bits = (uint8_t)((pattern >> ((4 - row) * 3)) & 7u);
            for (int column = 0; column < 3; ++column) {
                if ((row_bits & (uint8_t)(4u >> column)) == 0u) continue;
                for (uint8_t dy = 0u; dy < scale; ++dy) {
                    int target_y = y + (row * scale) + dy;
                    if ((target_y < 0) || (target_y >= height)) continue;
                    for (uint8_t dx = 0u; dx < scale; ++dx) {
                        int target_x = pen_x + (column * scale) + dx;
                        if ((target_x >= 0) && (target_x < width)) {
                            mask_set(bits, (uint32_t)target_y * width +
                                     (uint32_t)target_x);
                        }
                    }
                }
            }
        }
        pen_x += 4 * scale;
    }
    return true;
}

bool watch_text_rasterize_mask(uint8_t *bits, size_t bits_size,
                               uint16_t width, uint16_t height,
                               int x, int y, const char *text,
                               uint8_t scale)
{
    return watch_text_rasterize_mask_impl(bits, bits_size, width, height,
                                          x, y, text, scale, true);
}

bool watch_text_rasterize_mask_add(uint8_t *bits, size_t bits_size,
                                   uint16_t width, uint16_t height,
                                   int x, int y, const char *text,
                                   uint8_t scale)
{
    return watch_text_rasterize_mask_impl(bits, bits_size, width, height,
                                          x, y, text, scale, false);
}

