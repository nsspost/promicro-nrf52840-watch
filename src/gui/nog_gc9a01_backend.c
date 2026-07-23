#include "watch/nog_display.h"

#include <stdint.h>

#include "watch/gc9a01.h"

static bool write_solid_span(void *user,
                             int16_t x,
                             int16_t y,
                             uint16_t count,
                             uint32_t native_color)
{
    (void)user;
    return gc9a01_fill_rect((uint16_t)x,
                            (uint16_t)y,
                            count,
                            1u,
                            (uint16_t)native_color);
}

bool watch_nog_display_init(gno_context_t *context)
{
    const gno_backend_t backend = {
        .width = GC9A01_WIDTH,
        .height = GC9A01_HEIGHT,
        .format = GNO_PIXELFORMAT_RGB565,
        .user = 0,
        .begin = 0,
        .write_solid_span = write_solid_span,
        .write_span = 0,
        .end = 0
    };

    return gno_init_backend(context, &backend);
}
