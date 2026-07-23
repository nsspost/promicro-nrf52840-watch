/*
 * GC9A01A 240x240 SPI display driver.
 *
 * The initialization values are adapted from the BSD-licensed
 * Adafruit_GC9A01A library:
 *   https://github.com/adafruit/Adafruit_GC9A01A
 *
 * Written by Limor "ladyada" Fried for Adafruit Industries.
 * GC9A01A adaptation by Phil "PaintYourDragon" Burgess.
 * This file is an independent C implementation for the watch project.
 */

#include "watch/gc9a01.h"

#include "watch/board.h"

#include <stddef.h>
#include <stdint.h>

enum {
    CMD_SWRESET = 0x01,
    CMD_SLPOUT = 0x11,
    CMD_INVON = 0x21,
    CMD_DISPON = 0x29,
    CMD_CASET = 0x2A,
    CMD_RASET = 0x2B,
    CMD_RAMWR = 0x2C,
    CMD_MADCTL = 0x36,
    CMD_COLMOD = 0x3A,
    CMD_TEON = 0x35
};

#define INIT_DELAY 0x80u
#define INIT_END   0x00u

/*
 * Stream format: command, argument count (bit 7 requests 150 ms delay),
 * followed by arguments. A zero command terminates the stream.
 */
static const uint8_t init_commands[] = {
    0xEF, 0,
    0xEB, 1, 0x14,
    0xFE, 0,
    0xEF, 0,
    0xEB, 1, 0x14,
    0x84, 1, 0x40,
    0x85, 1, 0xFF,
    0x86, 1, 0xFF,
    0x87, 1, 0xFF,
    0x88, 1, 0x0A,
    0x89, 1, 0x21,
    0x8A, 1, 0x00,
    0x8B, 1, 0x80,
    0x8C, 1, 0x01,
    0x8D, 1, 0x01,
    0x8E, 1, 0xFF,
    0x8F, 1, 0xFF,
    0xB6, 2, 0x00, 0x00,
    CMD_MADCTL, 1, 0x48,
    CMD_COLMOD, 1, 0x05,
    0x90, 4, 0x08, 0x08, 0x08, 0x08,
    0xBD, 1, 0x06,
    0xBC, 1, 0x00,
    0xFF, 3, 0x60, 0x01, 0x04,
    0xC3, 1, 0x13,
    0xC4, 1, 0x13,
    0xC9, 1, 0x22,
    0xBE, 1, 0x11,
    0xE1, 2, 0x10, 0x0E,
    0xDF, 3, 0x21, 0x0C, 0x02,
    0xF0, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
    0xF1, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
    0xF2, 6, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,
    0xF3, 6, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,
    0xED, 2, 0x1B, 0x0B,
    0xAE, 1, 0x77,
    0xCD, 1, 0x63,
    0xE8, 1, 0x34,
    0x62, 12, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70,
              0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70,
    0x63, 12, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70,
              0x18, 0x13, 0x71, 0xF3, 0x70, 0x70,
    0x64, 7, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07,
    0x66, 10, 0x3C, 0x00, 0xCD, 0x67, 0x45,
              0x45, 0x10, 0x00, 0x00, 0x00,
    0x67, 10, 0x00, 0x3C, 0x00, 0x00, 0x00,
              0x01, 0x54, 0x10, 0x32, 0x98,
    0x74, 7, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00,
    0x98, 2, 0x3E, 0x07,
    CMD_TEON, 0,
    CMD_INVON, 0,
    CMD_SLPOUT, INIT_DELAY,
    CMD_DISPON, INIT_DELAY,
    INIT_END
};

static bool write_command(uint8_t command, const uint8_t *data, size_t length)
{
    bool ok;

    watch_lcd_select(true);
    watch_lcd_set_data_mode(false);
    ok = watch_lcd_write(&command, 1u);
    if (ok && (length > 0u)) {
        watch_lcd_set_data_mode(true);
        ok = watch_lcd_write(data, length);
    }
    watch_lcd_select(false);
    return ok;
}

static bool set_window(uint16_t x, uint16_t y, uint16_t width,
                       uint16_t height)
{
    uint16_t x2 = (uint16_t)(x + width - 1u);
    uint16_t y2 = (uint16_t)(y + height - 1u);
    uint8_t columns[4] = {
        (uint8_t)(x >> 8), (uint8_t)x,
        (uint8_t)(x2 >> 8), (uint8_t)x2
    };
    uint8_t rows[4] = {
        (uint8_t)(y >> 8), (uint8_t)y,
        (uint8_t)(y2 >> 8), (uint8_t)y2
    };

    return write_command(CMD_CASET, columns, sizeof(columns)) &&
           write_command(CMD_RASET, rows, sizeof(rows)) &&
           write_command(CMD_RAMWR, 0, 0u);
}

bool gc9a01_init(void)
{
    watch_lcd_select(false);
    watch_lcd_set_data_mode(true);
    watch_lcd_set_reset(true);
    watch_delay_ms(20u);
    watch_lcd_set_reset(false);
    watch_delay_ms(20u);
    watch_lcd_set_reset(true);
    watch_delay_ms(150u);

    if (!write_command(CMD_SWRESET, 0, 0u)) {
        return false;
    }
    watch_delay_ms(150u);

    const uint8_t *entry = init_commands;
    while (*entry != INIT_END) {
        uint8_t command = *entry++;
        uint8_t control = *entry++;
        size_t argument_count = control & 0x7Fu;

        if (!write_command(command, entry, argument_count)) {
            return false;
        }
        entry += argument_count;

        if ((control & INIT_DELAY) != 0u) {
            watch_delay_ms(150u);
        }
    }

    return true;
}

bool gc9a01_fill_rect(uint16_t x, uint16_t y, uint16_t width,
                      uint16_t height, uint16_t rgb565)
{
    static uint8_t scanline[GC9A01_WIDTH * 2u];

    if ((width == 0u) || (height == 0u) ||
        (x >= GC9A01_WIDTH) || (y >= GC9A01_HEIGHT) ||
        ((uint32_t)x + width > GC9A01_WIDTH) ||
        ((uint32_t)y + height > GC9A01_HEIGHT)) {
        return false;
    }

    for (uint16_t i = 0; i < width; ++i) {
        scanline[2u * i] = (uint8_t)(rgb565 >> 8);
        scanline[(2u * i) + 1u] = (uint8_t)rgb565;
    }

    if (!set_window(x, y, width, height)) {
        return false;
    }

    watch_lcd_select(true);
    watch_lcd_set_data_mode(true);
    for (uint16_t row = 0; row < height; ++row) {
        if (!watch_lcd_write(scanline, (size_t)width * 2u)) {
            watch_lcd_select(false);
            return false;
        }
    }
    watch_lcd_select(false);
    return true;
}

bool gc9a01_fill(uint16_t rgb565)
{
    return gc9a01_fill_rect(0u, 0u, GC9A01_WIDTH, GC9A01_HEIGHT, rgb565);
}

