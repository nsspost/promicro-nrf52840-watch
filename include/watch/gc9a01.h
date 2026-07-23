#ifndef WATCH_GC9A01_H
#define WATCH_GC9A01_H

#include <stdbool.h>
#include <stdint.h>

enum {
    GC9A01_WIDTH = 240,
    GC9A01_HEIGHT = 240
};

bool gc9a01_init(void);
bool gc9a01_fill(uint16_t rgb565);
bool gc9a01_fill_rect(uint16_t x, uint16_t y, uint16_t width,
                      uint16_t height, uint16_t rgb565);

#endif

