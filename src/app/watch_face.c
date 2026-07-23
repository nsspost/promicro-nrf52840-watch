#include "watch/watch_face.h"

#include <stddef.h>

#include "watch/watch_text.h"

#define SCREEN_SIZE 240

static const uint8_t digit_segments[10] = {
    0x3Fu, /* 0: A B C D E F */
    0x06u, /* 1: B C */
    0x5Bu, /* 2: A B D E G */
    0x4Fu, /* 3: A B C D G */
    0x66u, /* 4: B C F G */
    0x6Du, /* 5: A C D F G */
    0x7Du, /* 6: A C D E F G */
    0x07u, /* 7: A B C */
    0x7Fu, /* 8 */
    0x6Fu  /* 9 */
};

static gno_color_t background(const watch_face_t *face)
{
    return (face->screen == WATCH_SCREEN_HOME_CONTEXT) ?
           GNO_RGB(4, 10, 18) :
           GNO_RGB(0, 0, 0);
}

static gno_color_t foreground(const watch_face_t *face)
{
    return (face->screen == WATCH_SCREEN_HOME_CONTEXT) ?
           GNO_RGB(238, 244, 250) :
           GNO_RGB(75, 220, 255);
}

static gno_color_t accent(const watch_face_t *face)
{
    return (face->screen == WATCH_SCREEN_HOME_CONTEXT) ?
           GNO_RGB(64, 205, 150) :
           GNO_RGB(255, 170, 48);
}

static gno_color_t muted(void)
{
    return GNO_RGB(92, 106, 120);
}

static void draw_link_status(gno_context_t *graphics,
                             int x,
                             const char *label)
{
    watch_draw_text(graphics, x, 28, label, 2u, muted());
    gno_fill_rect(graphics,
                  x + watch_text_width(label, 2u) + 4,
                  31,
                  4,
                  4,
                  GNO_RGB(48, 58, 70));
}

static void draw_home_chrome(watch_face_t *face)
{
    gno_context_t *graphics = face->graphics;
    gno_color_t panel = GNO_RGB(10, 22, 34);

    draw_link_status(graphics, 42, "PH");
    draw_link_status(graphics, 96, "TS");
    draw_link_status(graphics, 148, "DET");
    gno_fill_rect(graphics, 36, 49, 168, 1, GNO_RGB(26, 46, 60));

    gno_fill_rect(graphics, 44, 174, 152, 36, panel);
    gno_fill_rect(graphics, 44, 174, 152, 1, GNO_RGB(24, 54, 64));
    gno_fill_rect(graphics, 44, 209, 152, 1, GNO_RGB(24, 54, 64));
    gno_fill_rect(graphics, 58, 189, 6, 6, accent(face));
    watch_draw_text(graphics, 75, 187, "LOCAL", 2u, foreground(face));
}

static void draw_digit_segments(gno_context_t *graphics,
                                int x,
                                int y,
                                uint8_t segments,
                                gno_color_t color)
{
    const int width = 36;
    const int height = 64;
    const int thickness = 7;
    const int middle_y = y + (height / 2) - (thickness / 2);
    if ((segments & (1u << 0)) != 0u) {
        gno_fill_rect(graphics, x + thickness, y,
                      width - (2 * thickness), thickness, color);
    }
    if ((segments & (1u << 1)) != 0u) {
        gno_fill_rect(graphics, x + width - thickness, y + thickness,
                      thickness, (height / 2) - thickness, color);
    }
    if ((segments & (1u << 2)) != 0u) {
        gno_fill_rect(graphics, x + width - thickness, middle_y + thickness,
                      thickness, (height / 2) - thickness, color);
    }
    if ((segments & (1u << 3)) != 0u) {
        gno_fill_rect(graphics, x + thickness, y + height - thickness,
                      width - (2 * thickness), thickness, color);
    }
    if ((segments & (1u << 4)) != 0u) {
        gno_fill_rect(graphics, x, middle_y + thickness,
                      thickness, (height / 2) - thickness, color);
    }
    if ((segments & (1u << 5)) != 0u) {
        gno_fill_rect(graphics, x, y + thickness,
                      thickness, (height / 2) - thickness, color);
    }
    if ((segments & (1u << 6)) != 0u) {
        gno_fill_rect(graphics, x + thickness, middle_y,
                      width - (2 * thickness), thickness, color);
    }
}

static uint8_t segments_for_digit(uint8_t digit)
{
    return (digit < 10u) ? digit_segments[digit] : 0u;
}

static void update_digit(gno_context_t *graphics,
                         int x,
                         int y,
                         uint8_t previous_digit,
                         uint8_t next_digit,
                         gno_color_t foreground_color,
                         gno_color_t background_color)
{
    uint8_t previous = segments_for_digit(previous_digit);
    uint8_t next = segments_for_digit(next_digit);

    /*
     * Light new segments first, then remove obsolete ones. No unchanged
     * segment is transferred and the position is never cleared as a whole.
     */
    draw_digit_segments(graphics,
                        x,
                        y,
                        (uint8_t)(next & (uint8_t)~previous),
                        foreground_color);
    draw_digit_segments(graphics,
                        x,
                        y,
                        (uint8_t)(previous & (uint8_t)~next),
                        background_color);
}

static void draw_hour_marks(gno_context_t *graphics, gno_color_t color)
{
    static const int16_t marks[12][4] = {
        {120, 12, 120, 25}, {174, 27, 167, 39},
        {213, 66, 201, 73}, {228, 120, 215, 120},
        {213, 174, 201, 167}, {174, 213, 167, 201},
        {120, 228, 120, 215}, {66, 213, 73, 201},
        {27, 174, 39, 167}, {12, 120, 25, 120},
        {27, 66, 39, 73}, {66, 27, 73, 39}
    };

    for (size_t i = 0; i < 12u; ++i) {
        gno_draw_line(graphics,
                      marks[i][0], marks[i][1],
                      marks[i][2], marks[i][3],
                      color);
    }
}

static void update_time(watch_face_t *face, watch_time_t time)
{
    gno_context_t *graphics = face->graphics;
    gno_color_t bg = background(face);
    gno_color_t fg = foreground(face);
    watch_time_t previous = face->displayed_time;
    int y = (face->screen == WATCH_SCREEN_HOME_CONTEXT) ? 67 : 76;

    update_digit(graphics, 34, y,
                 (previous.hour < 24u) ?
                    (uint8_t)(previous.hour / 10u) : 0xFFu,
                 (uint8_t)(time.hour / 10u), fg, bg);
    update_digit(graphics, 74, y,
                 (previous.hour < 24u) ?
                    (uint8_t)(previous.hour % 10u) : 0xFFu,
                 (uint8_t)(time.hour % 10u), fg, bg);
    update_digit(graphics, 130, y,
                 (previous.minute < 60u) ?
                    (uint8_t)(previous.minute / 10u) : 0xFFu,
                 (uint8_t)(time.minute / 10u), fg, bg);
    update_digit(graphics, 170, y,
                 (previous.minute < 60u) ?
                    (uint8_t)(previous.minute % 10u) : 0xFFu,
                 (uint8_t)(time.minute % 10u), fg, bg);

    if ((previous.hour >= 24u) || (previous.minute >= 60u)) {
        gno_fill_rect(graphics, 116, y + 18, 7, 7, accent(face));
        gno_fill_rect(graphics, 116, y + 42, 7, 7, accent(face));
    }
}

static int seconds_progress(watch_time_t time)
{
    const int width = 152;
    const int ticks_per_minute = 60 * WATCH_CLOCK_SUBSECOND_HZ;
    int phase = ((int)time.second * WATCH_CLOCK_SUBSECOND_HZ) +
                time.subsecond;

    /*
     * Round upward so all 152 one-pixel steps are distributed over a minute.
     * At 8 Hz a visible step occurs roughly every 0.4 seconds.
     */
    return ((phase * width) + ticks_per_minute - 1) / ticks_per_minute;
}

static void update_seconds(watch_face_t *face, watch_time_t time)
{
    gno_context_t *graphics = face->graphics;
    const int x = 44;
    const int y =
        (face->screen == WATCH_SCREEN_HOME_CONTEXT) ? 151 : 160;
    const int width = 152;
    const gno_color_t track = GNO_RGB(28, 34, 42);
    int next = seconds_progress(time);

    if ((face->displayed_time.second >= 60u) ||
        (face->displayed_time.subsecond >= WATCH_CLOCK_SUBSECOND_HZ)) {
        gno_fill_rect(graphics, x, y, width, 6, track);
        if (next > 0) {
            gno_fill_rect(graphics, x, y, next, 6, accent(face));
        }
        return;
    }

    int previous = seconds_progress(face->displayed_time);
    if (next > previous) {
        gno_fill_rect(graphics,
                      x + previous,
                      y,
                      next - previous,
                      6,
                      accent(face));
    } else if (next < previous) {
        gno_fill_rect(graphics,
                      x + next,
                      y,
                      previous - next,
                      6,
                      track);
    }
}

static bool finish_drawing(gno_context_t *graphics)
{
    if (gno_has_error(graphics)) {
        gno_reset_error(graphics);
        return false;
    }
    return true;
}

bool watch_face_init(watch_face_t *face, gno_context_t *graphics)
{
    if ((face == 0) || (graphics == 0)) {
        return false;
    }

    face->graphics = graphics;
    face->displayed_time.hour = 0xFFu;
    face->displayed_time.minute = 0xFFu;
    face->displayed_time.second = 0xFFu;
    face->displayed_time.subsecond = 0xFFu;
    face->initialized = true;
    face->screen = WATCH_SCREEN_HOME_CONTEXT;
    return true;
}

bool watch_face_update(watch_face_t *face, watch_time_t time)
{
    if ((face == 0) || !face->initialized) {
        return false;
    }

    if ((time.hour != face->displayed_time.hour) ||
        (time.minute != face->displayed_time.minute)) {
        update_time(face, time);
    }
    if ((time.second != face->displayed_time.second) ||
        (time.subsecond != face->displayed_time.subsecond)) {
        update_seconds(face, time);
    }

    face->displayed_time = time;
    return finish_drawing(face->graphics);
}

static bool show_screen(watch_face_t *face, watch_screen_t screen)
{
    if ((face == 0) || !face->initialized) {
        return false;
    }

    if (face->screen == screen &&
        face->displayed_time.hour < 24u) {
        return true;
    }

    face->screen = screen;
    face->displayed_time.hour = 0xFFu;
    face->displayed_time.minute = 0xFFu;
    face->displayed_time.second = 0xFFu;
    face->displayed_time.subsecond = 0xFFu;
    gno_clear(face->graphics, background(face));

    if (screen == WATCH_SCREEN_HOME_CONTEXT) {
        draw_home_chrome(face);
    } else {
        draw_hour_marks(face->graphics, GNO_RGB(70, 78, 88));
    }
    return finish_drawing(face->graphics);
}

bool watch_face_show_home(watch_face_t *face)
{
    return show_screen(face, WATCH_SCREEN_HOME_CONTEXT);
}

bool watch_face_show_diagnostic(watch_face_t *face)
{
    return show_screen(face, WATCH_SCREEN_DIAGNOSTIC_FACE);
}
