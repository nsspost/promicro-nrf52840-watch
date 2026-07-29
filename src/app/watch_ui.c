#include "watch/watch_ui.h"

#include <stddef.h>

#include "watch/strict_context_icons.h"
#include "watch/strict_context_text.h"
#include "watch/watch_text.h"

enum {
    SCREEN_SIZE = 240,
    MINIMUM_HIT_SIZE = 44,
    HOLD_TICKS = 12,
    COMMAND_RESULT_TICKS = 8,
    CRITICAL_RESULT_TICKS = 16,
    TICKS_PER_DAY = 24 * 60 * 60 * WATCH_CLOCK_SUBSECOND_HZ
};

typedef enum {
    ACTION_NONE = 0,
    ACTION_BACK,
    ACTION_OPEN_DEVICES,
    ACTION_OPEN_OVERVIEW,
    ACTION_OPEN_PARAMETERS,
    ACTION_OPEN_METRIC,
    ACTION_OPEN_CONTROLS,
    ACTION_OPEN_COMMAND,
    ACTION_SUBMIT_COMMAND,
    ACTION_NEXT_COMMAND_CASE,
    ACTION_OPEN_CRITICAL,
    ACTION_SUBMIT_CRITICAL,
    ACTION_NEXT_CRITICAL_CASE,
    ACTION_OPEN_EVENTS,
    ACTION_OPEN_EVENT,
    ACTION_ACK_EVENT,
    ACTION_OPEN_DIAGNOSTIC,
    ACTION_TOGGLE_SKIN,
    ACTION_OPEN_MEDIA,
    ACTION_MEDIA_PREVIOUS,
    ACTION_MEDIA_PLAY_PAUSE,
    ACTION_MEDIA_NEXT
} ui_action_t;

static watch_ui_skin_t active_skin = WATCH_UI_SKIN_STRICT_CONTEXT;

static watch_strict_text_style_t strict_style_from_scale(uint8_t scale)
{
    if (scale >= 9u) {
        return WATCH_STRICT_TEXT_TIME;
    }
    if (scale >= 6u) {
        return WATCH_STRICT_TEXT_DEVICE;
    }
    if (scale >= 4u) {
        return WATCH_STRICT_TEXT_STATUS;
    }
    if (scale >= 3u) {
        return WATCH_STRICT_TEXT_TITLE;
    }
    if (scale >= 2u) {
        return WATCH_STRICT_TEXT_ROW;
    }
    return WATCH_STRICT_TEXT_CAPTION;
}

static void draw_skin_text(gno_context_t *graphics,
                           int x,
                           int y,
                           const char *text,
                           uint8_t scale,
                           gno_color_t color)
{
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        watch_strict_draw_text(graphics, x, y, text,
                               strict_style_from_scale(scale), color);
        return;
    }
    watch_draw_text(graphics, x, y, text, scale, color);
}

static int skin_text_width(const char *text, uint8_t scale)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           watch_strict_text_width(text, strict_style_from_scale(scale)) :
           watch_text_width(text, scale);
}

#define watch_draw_text draw_skin_text

static gno_color_t color_background(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(0, 0, 0) : GNO_RGB(4, 10, 18);
}

static gno_color_t color_surface(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(0, 0, 0) : GNO_RGB(10, 22, 34);
}

static gno_color_t color_surface_high(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(5, 5, 5) : GNO_RGB(18, 34, 48);
}

static gno_color_t color_line(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(102, 102, 102) : GNO_RGB(35, 64, 78);
}

static gno_color_t color_text(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(255, 255, 255) : GNO_RGB(238, 244, 250);
}

static gno_color_t color_muted(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(160, 160, 160) : GNO_RGB(100, 119, 136);
}

static gno_color_t color_accent(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(243, 244, 239) : GNO_RGB(64, 205, 150);
}

static gno_color_t color_info(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(243, 244, 239) : GNO_RGB(75, 190, 255);
}

static gno_color_t color_warning(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(243, 244, 239) : GNO_RGB(255, 178, 64);
}

static gno_color_t color_danger(void)
{
    return (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
           GNO_RGB(255, 255, 255) : GNO_RGB(255, 82, 92);
}

static uint32_t time_ticks(watch_time_t time)
{
    uint32_t seconds =
        ((uint32_t)time.hour * 3600u) +
        ((uint32_t)time.minute * 60u) +
        (uint32_t)time.second;
    return (seconds * WATCH_CLOCK_SUBSECOND_HZ) + time.subsecond;
}

static uint32_t elapsed_ticks(uint32_t start, uint32_t now)
{
    return (now >= start) ? (now - start) :
           ((uint32_t)TICKS_PER_DAY - start + now);
}

static bool point_in_rect(uint16_t x, uint16_t y,
                          const watch_ui_hit_target_t *target)
{
    return ((int32_t)x >= target->x) &&
           ((int32_t)y >= target->y) &&
           ((int32_t)x < (int32_t)target->x + target->w) &&
           ((int32_t)y < (int32_t)target->y + target->h);
}

static void add_hit(watch_ui_t *ui,
                    int x,
                    int y,
                    int w,
                    int h,
                    ui_action_t action,
                    uint8_t argument,
                    bool requires_hold)
{
    if (ui->hit_count >= WATCH_UI_MAX_HIT_TARGETS) {
        return;
    }

    if (w < MINIMUM_HIT_SIZE) {
        x -= (MINIMUM_HIT_SIZE - w) / 2;
        w = MINIMUM_HIT_SIZE;
    }
    if (h < MINIMUM_HIT_SIZE) {
        y -= (MINIMUM_HIT_SIZE - h) / 2;
        h = MINIMUM_HIT_SIZE;
    }

    ui->hits[ui->hit_count] = (watch_ui_hit_target_t) {
        .x = (int16_t)x,
        .y = (int16_t)y,
        .w = (int16_t)w,
        .h = (int16_t)h,
        .action = (uint8_t)action,
        .argument = argument,
        .requires_hold = requires_hold
    };
    ui->hit_count++;
}

static void text_center(gno_context_t *graphics,
                        int center_x,
                        int y,
                        const char *text,
                        uint8_t scale,
                        gno_color_t color)
{
    int width = skin_text_width(text, scale);
    watch_draw_text(graphics, center_x - (width / 2), y,
                    text, scale, color);
}

static void draw_card(gno_context_t *graphics,
                      int x,
                      int y,
                      int w,
                      int h,
                      gno_color_t border)
{
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        gno_draw_hline(graphics, x, y + h - 1, w, color_line());
        return;
    }
    gno_fill_rect(graphics, x, y, w, h, color_surface());
    gno_draw_rect(graphics, x, y, w, h, border);
}

static void draw_button(watch_ui_t *ui,
                        int x,
                        int y,
                        int w,
                        int h,
                        const char *label,
                        gno_color_t border,
                        ui_action_t action,
                        uint8_t argument,
                        bool requires_hold)
{
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        bool inverse =
            action == ACTION_ACK_EVENT ||
            action == ACTION_NEXT_COMMAND_CASE ||
            action == ACTION_NEXT_CRITICAL_CASE ||
            action == ACTION_TOGGLE_SKIN;
        watch_strict_text_style_t button_style = WATCH_STRICT_TEXT_ROW;
        if (watch_strict_text_width(label, button_style) > (w - 16)) {
            button_style = WATCH_STRICT_TEXT_BODY;
        }
        int text_width = watch_strict_text_width(label, button_style);
        int text_y = y + ((h -
            watch_strict_text_line_height(button_style)) / 2);
        if (inverse) {
            gno_fill_rect(ui->graphics, x, y, w, h, color_text());
            watch_strict_draw_text(ui->graphics,
                                   x + ((w - text_width) / 2), text_y,
                                   label, button_style, color_background());
        } else {
            gno_draw_rect(ui->graphics, x, y, w, h, color_text());
            watch_strict_draw_text(ui->graphics,
                                   x + ((w - text_width) / 2), text_y,
                                   label, button_style, color_text());
        }
        add_hit(ui, x, y, w, h, action, argument, requires_hold);
        return;
    }
    draw_card(ui->graphics, x, y, w, h, border);
    text_center(ui->graphics, x + (w / 2), y + (h / 2) - 5,
                label, 2u, color_text());
    add_hit(ui, x, y, w, h, action, argument, requires_hold);
}

static void draw_header(watch_ui_t *ui,
                        const char *title,
                        bool show_back,
                        gno_color_t accent)
{
    gno_context_t *graphics = ui->graphics;
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        gno_fill_rect(graphics, 26, 61, 188, 2, color_line());
        watch_strict_text_style_t title_style = WATCH_STRICT_TEXT_TITLE;
        if (watch_strict_text_width(title, title_style) > 140) {
            title_style = WATCH_STRICT_TEXT_ROW;
        }
        int title_width = watch_strict_text_width(title, title_style);
        watch_strict_draw_text(graphics, 120 - (title_width / 2), 26,
                               title, title_style, color_text());
        if (show_back) {
            watch_strict_draw_icon(graphics, 27, 20,
                                   WATCH_STRICT_ICON_CARET_LEFT,
                                   color_text());
            add_hit(ui, 12, 6, 44, 44, ACTION_BACK, 0u, false);
        }
        return;
    }
    gno_fill_rect(graphics, 28, 40, 184, 1, color_line());
    text_center(graphics, 120, 20, title, 2u, color_text());
    gno_fill_rect(graphics, 112, 39, 16, 2, accent);

    if (show_back) {
        watch_draw_text(graphics, 31, 20, "НАЗАД", 1u, color_muted());
        add_hit(ui, 22, 6, 58, 42, ACTION_BACK, 0u, false);
    }
}

static void draw_quality(gno_context_t *graphics,
                         int x,
                         int y,
                         watch_ui_quality_t quality)
{
    gno_color_t color = color_accent();
    if (quality == WATCH_UI_QUALITY_STALE ||
        quality == WATCH_UI_QUALITY_UNCERTAIN) {
        color = color_warning();
    } else if (quality == WATCH_UI_QUALITY_BAD ||
               quality == WATCH_UI_QUALITY_OFFLINE) {
        color = color_danger();
    }

    gno_fill_rect(graphics, x, y + 2, 5, 5, color);
    watch_draw_text(graphics, x + 10, y,
                    watch_ui_quality_label(quality), 1u, color);
}

static void reverse_chars(char *text, uint8_t length)
{
    for (uint8_t i = 0u; i < (uint8_t)(length / 2u); ++i) {
        char saved = text[i];
        text[i] = text[length - i - 1u];
        text[length - i - 1u] = saved;
    }
}

static void append_unsigned(char *text, uint8_t *length, uint16_t value)
{
    uint8_t start = *length;
    do {
        text[*length] = (char)('0' + (value % 10u));
        (*length)++;
        value /= 10u;
    } while (value != 0u);
    reverse_chars(&text[start], (uint8_t)(*length - start));
}

static void format_tenths(int16_t value, char text[12])
{
    uint8_t length = 0u;
    uint16_t magnitude;

    if (value < 0) {
        text[length++] = '-';
        magnitude = (uint16_t)(-value);
    } else {
        magnitude = (uint16_t)value;
    }

    append_unsigned(text, &length, (uint16_t)(magnitude / 10u));
    text[length++] = '.';
    text[length++] = (char)('0' + (magnitude % 10u));
    text[length] = '\0';
}

static void format_percent_from_tenths(int16_t value, char text[6])
{
    uint16_t percent = 0u;
    uint8_t length = 0u;

    if (value > 0) {
        percent = (uint16_t)(value / 10);
    }
    if (percent > 100u) {
        percent = 100u;
    }

    append_unsigned(text, &length, percent);
    text[length++] = '%';
    text[length] = '\0';
}

static const char *phone_state_label(watch_ui_phone_state_t state)
{
    switch (state) {
        case WATCH_UI_PHONE_ADVERTISING: return "ADV";
        case WATCH_UI_PHONE_CONNECTED: return "CON";
        case WATCH_UI_PHONE_SUBSCRIBED: return "SUB";
        case WATCH_UI_PHONE_READY: return "READY";
        case WATCH_UI_PHONE_ERROR: return "ERR";
        case WATCH_UI_PHONE_OFF:
        default:
            return "OFF";
    }
}

static bool phone_state_online(watch_ui_phone_state_t state)
{
    return state == WATCH_UI_PHONE_CONNECTED ||
           state == WATCH_UI_PHONE_SUBSCRIBED ||
           state == WATCH_UI_PHONE_READY;
}

static void draw_metric_value(gno_context_t *graphics,
                              int center_x,
                              int y,
                              const watch_ui_metric_t *metric,
                              uint8_t scale)
{
    char value[12];
    format_tenths(metric->value_tenths, value);

    int value_width = skin_text_width(value, scale);
    int unit_width = skin_text_width(metric->unit, 1u);
    int gap = (metric->unit[0] != '\0') ? 6 : 0;
    int left = center_x - ((value_width + gap + unit_width) / 2);
    watch_draw_text(graphics, left, y, value, scale, color_text());
    if (metric->unit[0] != '\0') {
        watch_draw_text(graphics, left + value_width + gap,
                        y + ((int)scale * 2), metric->unit, 1u,
                        color_muted());
    }
}

static void draw_status_pill(gno_context_t *graphics,
                             const char *status,
                             bool online)
{
    gno_color_t color = online ? color_accent() : color_danger();
    int width = skin_text_width(status, 1u) + 24;
    int x = 120 - (width / 2);
    gno_draw_rect(graphics, x, 48, width, 18, color);
    gno_fill_rect(graphics, x + 7, 54, 5, 5, color);
    watch_draw_text(graphics, x + 17, 52, status, 1u, color_text());
}

static void reset_hits(watch_ui_t *ui)
{
    ui->hit_count = 0u;
    ui->pressed_hit = -1;
    ui->hold_active = false;
    ui->hold_progress = 0u;
}

static void draw_home_time_digit(gno_context_t *graphics,
                                 int x,
                                 int y,
                                 uint8_t digit,
                                 gno_color_t color)
{
    static const uint8_t digit_segments[10] = {
        0x3Fu, 0x06u, 0x5Bu, 0x4Fu, 0x66u,
        0x6Du, 0x7Du, 0x07u, 0x7Fu, 0x6Fu
    };
    const int width = 31;
    const int height = 53;
    const int thickness = 6;
    const int middle = y + (height / 2) - (thickness / 2);
    uint8_t segments = (digit < 10u) ? digit_segments[digit] : 0u;

    if ((segments & 0x01u) != 0u) {
        gno_fill_rect(graphics, x + thickness, y,
                      width - (2 * thickness), thickness, color);
    }
    if ((segments & 0x02u) != 0u) {
        gno_fill_rect(graphics, x + width - thickness, y + thickness,
                      thickness, (height / 2) - thickness, color);
    }
    if ((segments & 0x04u) != 0u) {
        gno_fill_rect(graphics, x + width - thickness, middle + thickness,
                      thickness, (height / 2) - thickness, color);
    }
    if ((segments & 0x08u) != 0u) {
        gno_fill_rect(graphics, x + thickness, y + height - thickness,
                      width - (2 * thickness), thickness, color);
    }
    if ((segments & 0x10u) != 0u) {
        gno_fill_rect(graphics, x, middle + thickness,
                      thickness, (height / 2) - thickness, color);
    }
    if ((segments & 0x20u) != 0u) {
        gno_fill_rect(graphics, x, y + thickness,
                      thickness, (height / 2) - thickness, color);
    }
    if ((segments & 0x40u) != 0u) {
        gno_fill_rect(graphics, x + thickness, middle,
                      width - (2 * thickness), thickness, color);
    }
}

static void draw_home_time(watch_ui_t *ui, watch_time_t time)
{
    gno_context_t *graphics = ui->graphics;
    gno_fill_rect(graphics, 30, 52, 180, 60, color_background());
    draw_home_time_digit(graphics, 34, 55,
                         (uint8_t)(time.hour / 10u), color_text());
    draw_home_time_digit(graphics, 68, 55,
                         (uint8_t)(time.hour % 10u), color_text());
    gno_fill_rect(graphics, 112, 72, 6, 6, color_accent());
    gno_fill_rect(graphics, 112, 91, 6, 6, color_accent());
    draw_home_time_digit(graphics, 131, 55,
                         (uint8_t)(time.minute / 10u), color_text());
    draw_home_time_digit(graphics, 165, 55,
                         (uint8_t)(time.minute % 10u), color_text());
}

static void draw_home_progress(watch_ui_t *ui, watch_time_t time)
{
    int progress =
        ((((int)time.second * WATCH_CLOCK_SUBSECOND_HZ) + time.subsecond) *
         176) / (60 * WATCH_CLOCK_SUBSECOND_HZ);
    gno_fill_rect(ui->graphics, 32, 119, 176, 4, GNO_RGB(28, 42, 52));
    if (progress > 0) {
        gno_fill_rect(ui->graphics, 32, 119, progress, 4, color_accent());
    }
}

static void draw_home(watch_ui_t *ui, watch_time_t time)
{
    gno_context_t *graphics = ui->graphics;
    const watch_ui_device_t *pump = &ui->model.devices[0];

    watch_draw_text(graphics, 42, 25, "PH", 1u, color_muted());
    gno_fill_rect(graphics, 54, 27, 4, 4, color_muted());
    watch_draw_text(graphics, 101, 25, "TS", 1u, color_muted());
    gno_fill_rect(graphics, 113, 27, 4, 4, color_muted());
    watch_draw_text(graphics, 157, 25, "DET", 1u, color_muted());
    gno_fill_rect(graphics, 175, 27, 4, 4, color_muted());
    gno_fill_rect(graphics, 32, 42, 176, 1, color_line());

    draw_home_time(ui, time);
    draw_home_progress(ui, time);

    draw_card(graphics, 28, 132, 184, 44, color_line());
    gno_fill_rect(graphics, 38, 145, 6, 18, color_accent());
    watch_draw_text(graphics, 53, 140, pump->source_id, 2u, color_text());
    draw_metric_value(graphics, 166, 143, &pump->metrics[0], 2u);
    add_hit(ui, 24, 128, 192, 52, ACTION_OPEN_OVERVIEW, 0u, false);

    draw_button(ui, 28, 184, 88, 34, "УСТРОЙСТВА", color_info(),
                ACTION_OPEN_DEVICES, 0u, false);
    draw_button(ui, 124, 184, 88, 34, "СОБЫТИЯ", color_warning(),
                ACTION_OPEN_EVENTS, 0u, false);
    add_hit(ui, 88, 6, 64, 38, ACTION_OPEN_DIAGNOSTIC, 0u, false);
}

static void draw_strict_home(watch_ui_t *ui, watch_time_t time)
{
    gno_context_t *graphics = ui->graphics;
    const watch_ui_device_t *device = &ui->model.devices[0];
    const watch_ui_device_t *local = &ui->model.devices[2];
    char battery_text[6];
    char packet_text[12];
    char clock_text[6] = {
        (char)('0' + (time.hour / 10u)),
        (char)('0' + (time.hour % 10u)),
        ':',
        (char)('0' + (time.minute / 10u)),
        (char)('0' + (time.minute % 10u)),
        '\0'
    };

    format_percent_from_tenths(local->metrics[0].value_tenths, battery_text);

    text_center(graphics, 120, 5, clock_text, 9u, color_text());
    gno_fill_rect(graphics, 26, 83, 188, 2, color_line());

    watch_strict_draw_icon(graphics, 34, 87,
                           WATCH_STRICT_ICON_BATTERY, color_text());
    watch_draw_text(graphics, 68, 95, battery_text, 4u, color_text());
    gno_fill_rect(graphics, 119, 88, 2, 31, color_line());
    watch_strict_draw_icon(graphics, 126, 87,
                           WATCH_STRICT_ICON_CLOUD, color_text());
    watch_draw_text(graphics, 160, 92,
                    phone_state_label(ui->phone.state), 3u,
                    phone_state_online(ui->phone.state) ?
                    color_text() : color_muted());
    {
        uint8_t length = 0u;
        uint32_t packets =
            ui->phone.received_packets + ui->phone.tx_notifications;
        if (packets > 999u) {
            packets = 999u;
        }
        append_unsigned(packet_text, &length, (uint16_t)packets);
        packet_text[length] = '\0';
    }
    watch_draw_text(graphics, 160, 112, packet_text, 1u, color_muted());
    if (ui->phone.state == WATCH_UI_PHONE_READY) {
        gno_fill_rect(graphics, 190, 95, 5, 5, color_text());
    } else {
        gno_draw_rect(graphics, 190, 95, 5, 5, color_muted());
    }

    gno_fill_rect(graphics, 26, 125, 188, 2, color_line());
    if (device->online) {
        watch_draw_text(graphics, 26, 139,
                        device->source_id, 6u, color_text());
        watch_strict_draw_icon(graphics, 190, 143,
                               WATCH_STRICT_ICON_CARET_RIGHT, color_text());
        add_hit(ui, 24, 124, 192, 68,
                ACTION_OPEN_OVERVIEW, 0u, false);
    }

    watch_strict_draw_icon(graphics, 45, 178,
                           WATCH_STRICT_ICON_CHAT, color_text());
    watch_draw_text(graphics, 75, 189, "2", 3u, color_text());
    text_center(graphics, 120, 190, "3 рядом", 2u, color_muted());
    watch_strict_draw_icon(graphics, 157, 178,
                           WATCH_STRICT_ICON_ECOSYSTEM, color_text());
    gno_fill_rect(graphics, 171, 192, 4, 4, color_text());
    watch_draw_text(graphics, 184, 189, "1", 3u, color_text());

    add_hit(ui, 75, 196, 90, 44, ACTION_OPEN_DEVICES, 0u, false);
    add_hit(ui, 165, 196, 51, 44, ACTION_OPEN_EVENTS, 0u, false);
    add_hit(ui, 24, 196, 44, 44, ACTION_OPEN_MEDIA, 0u, false);
    add_hit(ui, 88, 3, 64, 44, ACTION_OPEN_DIAGNOSTIC, 0u, false);
}

static void draw_strict_device_list(watch_ui_t *ui)
{
    draw_header(ui, "3 РЯДОМ", true, color_text());
    for (uint8_t i = 0u; i < ui->model.device_count; ++i) {
        const watch_ui_device_t *device = &ui->model.devices[i];
        int y = 64 + ((int)i * 55);
        gno_draw_hline(ui->graphics, 26, y + 54, 188, color_line());
        watch_draw_text(ui->graphics, 29, y + 8,
                        device->source_id, 3u, color_text());
        watch_draw_text(ui->graphics, 29, y + 33,
                        device->location, 2u, color_muted());
        watch_draw_text(ui->graphics, 151, y + 21,
                        device->online ? "НОРМА" : "НЕТ СВЯЗИ",
                        1u, color_muted());
        watch_strict_draw_icon(ui->graphics, 181, y + 11,
                               WATCH_STRICT_ICON_CARET_RIGHT, color_text());
        add_hit(ui, 24, y, 192, 55,
                ACTION_OPEN_OVERVIEW, i, false);
    }
}

static void draw_strict_overview(watch_ui_t *ui)
{
    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    draw_header(ui, device->source_id, true, color_text());

    watch_strict_draw_icon(ui->graphics, 27, 65,
                           device->online ? WATCH_STRICT_ICON_CHECK :
                           WATCH_STRICT_ICON_ERROR,
                           color_text());
    watch_draw_text(ui->graphics, 61, 76,
                    device->status, 4u, color_text());
    gno_draw_hline(ui->graphics, 26, 108, 188, color_line());

    for (uint8_t i = 0u; i < 2u && i < device->metric_count; ++i) {
        const watch_ui_metric_t *metric = &device->metrics[i];
        int x = (i == 0u) ? 27 : 123;
        watch_draw_text(ui->graphics, x, 116,
                        metric->compact_label, 1u, color_muted());
        draw_metric_value(ui->graphics, x + 43, 136, metric, 4u);
    }
    gno_fill_rect(ui->graphics, 119, 110, 1, 62, color_line());
    gno_draw_hline(ui->graphics, 26, 173, 188, color_line());

    watch_strict_draw_icon(ui->graphics, 56, 171,
                           WATCH_STRICT_ICON_LIST, color_text());
    text_center(ui->graphics, 72, 194, "ПАРАМЕТРЫ", 1u, color_text());
    gno_fill_rect(ui->graphics, 119, 174, 1, 49, color_line());
    watch_strict_draw_icon(ui->graphics, 152, 171,
                           WATCH_STRICT_ICON_SLIDERS, color_text());
    text_center(ui->graphics, 168, 194, "УПРАВЛЕНИЕ", 1u, color_text());
    add_hit(ui, 24, 173, 96, 50,
            ACTION_OPEN_PARAMETERS, 0u, false);
    add_hit(ui, 120, 173, 96, 50,
            ACTION_OPEN_CONTROLS, 0u, false);
}

static void draw_strict_parameter_list(watch_ui_t *ui)
{
    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    draw_header(ui, "ПАРАМЕТРЫ", true, color_text());

    for (uint8_t i = 0u; i < device->metric_count; ++i) {
        const watch_ui_metric_t *metric = &device->metrics[i];
        int y = 64 + ((int)i * 55);
        gno_draw_hline(ui->graphics, 26, y + 54, 188, color_line());
        watch_draw_text(ui->graphics, 30, y + 10,
                        metric->compact_label, 2u, color_text());
        if (i == 2u) {
            watch_draw_text(ui->graphics, 159, y + 10,
                            device->mode, 2u, color_text());
        } else {
            draw_metric_value(ui->graphics, 177, y + 9, metric, 3u);
        }
        watch_draw_text(ui->graphics, 30, y + 33,
                        watch_ui_quality_label(metric->quality),
                        1u, color_muted());
        add_hit(ui, 24, y, 192, 55,
                ACTION_OPEN_METRIC, i, false);
    }
}

static void draw_strict_control_list(watch_ui_t *ui)
{
    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    draw_header(ui, "УПРАВЛЕНИЕ", true, color_text());

    watch_strict_draw_icon(ui->graphics, 27, 68,
                           WATCH_STRICT_ICON_CHECK, color_text());
    watch_draw_text(ui->graphics, 62, 79,
                    device->status, 4u, color_text());
    gno_draw_hline(ui->graphics, 26, 111, 188, color_line());

    watch_strict_draw_icon(ui->graphics, 27, 119,
                           WATCH_STRICT_ICON_LOCK, color_text());
    watch_draw_text(ui->graphics, 62, 119, "ЗАЩИЩЕННАЯ", 1u,
                    color_muted());
    watch_draw_text(ui->graphics, 62, 138, "РЕЖИМ AUTO", 2u,
                    color_text());
    watch_strict_draw_icon(ui->graphics, 181, 124,
                           WATCH_STRICT_ICON_CARET_RIGHT, color_text());
    gno_draw_hline(ui->graphics, 26, 169, 188, color_line());

    watch_strict_draw_icon(ui->graphics, 27, 176,
                           WATCH_STRICT_ICON_STOP, color_text());
    watch_draw_text(ui->graphics, 62, 176, "КРИТИЧЕСКАЯ", 1u,
                    color_muted());
    watch_draw_text(ui->graphics, 62, 195, "ОСТАНОВ", 3u,
                    color_text());
    watch_strict_draw_icon(ui->graphics, 181, 181,
                           WATCH_STRICT_ICON_CARET_RIGHT, color_text());
    gno_draw_hline(ui->graphics, 26, 226, 188, color_line());

    add_hit(ui, 24, 112, 192, 58,
            ACTION_OPEN_COMMAND, 0u, false);
    add_hit(ui, 24, 170, 192, 58,
            ACTION_OPEN_CRITICAL, 0u, false);
}

static void draw_device_list(watch_ui_t *ui)
{
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        draw_strict_device_list(ui);
        return;
    }
    draw_header(ui, "УСТРОЙСТВА", true, color_info());

    for (uint8_t i = 0u; i < ui->model.device_count; ++i) {
        const watch_ui_device_t *device = &ui->model.devices[i];
        int y = 50 + ((int)i * 49);
        gno_color_t border = device->online ? color_line() : color_danger();

        draw_card(ui->graphics, 28, y, 184, 41, border);
        gno_fill_rect(ui->graphics, 38, y + 11, 6, 19,
                      device->online ? color_accent() : color_danger());
        watch_draw_text(ui->graphics, 54, y + 8,
                        device->source_id, 2u, color_text());
        watch_draw_text(ui->graphics, 54, y + 28,
                        device->location, 1u, color_muted());
        watch_draw_text(ui->graphics, 162, y + 28,
                        device->online ? "ВКЛ" : "НЕТ", 1u,
                        device->online ? color_accent() : color_danger());
        add_hit(ui, 24, y - 2, 192, 45,
                ACTION_OPEN_OVERVIEW, i, false);
    }
}

static void draw_overview(watch_ui_t *ui)
{
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        draw_strict_overview(ui);
        return;
    }
    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    draw_header(ui, device->source_id, true,
                device->online ? color_accent() : color_danger());
    draw_status_pill(ui->graphics, device->status, device->online);

    for (uint8_t i = 0u; i < 2u && i < device->metric_count; ++i) {
        const watch_ui_metric_t *metric = &device->metrics[i];
        int x = (i == 0u) ? 28 : 124;
        gno_color_t border =
            (metric->quality == WATCH_UI_QUALITY_GOOD) ?
            color_line() : color_warning();
        draw_card(ui->graphics, x, 73, 88, 65, border);
        text_center(ui->graphics, x + 44, 81,
                    metric->compact_label, 1u, color_muted());
        draw_metric_value(ui->graphics, x + 44, 99, metric, 2u);
        draw_quality(ui->graphics, x + 12, 124, metric->quality);
        add_hit(ui, x, 69, 88, 73, ACTION_OPEN_METRIC, i, false);
    }

    draw_card(ui->graphics, 28, 146, 184, 29, color_line());
    watch_draw_text(ui->graphics, 39, 154, "РЕЖИМ", 1u, color_muted());
    text_center(ui->graphics, 164, 152, device->mode, 2u, color_text());

    draw_button(ui, 28, 184, 88, 34, "ПАРАМЕТРЫ", color_info(),
                ACTION_OPEN_PARAMETERS, 0u, false);
    draw_button(ui, 124, 184, 88, 34, "УПРАВЛЕНИЕ", color_warning(),
                ACTION_OPEN_CONTROLS, 0u, false);
}

static void draw_parameter_list(watch_ui_t *ui)
{
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        draw_strict_parameter_list(ui);
        return;
    }
    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    draw_header(ui, "ПАРАМЕТРЫ", true, color_info());

    for (uint8_t i = 0u; i < device->metric_count; ++i) {
        const watch_ui_metric_t *metric = &device->metrics[i];
        int y = 49 + ((int)i * 47);
        draw_card(ui->graphics, 28, y, 184, 39, color_line());
        watch_draw_text(ui->graphics, 39, y + 7,
                        metric->compact_label, 2u, color_text());
        if (i == 2u) {
            watch_draw_text(ui->graphics, 154, y + 8,
                            device->mode, 2u, color_text());
        } else {
            draw_metric_value(ui->graphics, 171, y + 8, metric, 2u);
        }
        draw_quality(ui->graphics, 39, y + 27, metric->quality);
        add_hit(ui, 24, y - 2, 192, 43,
                ACTION_OPEN_METRIC, i, false);
    }

    draw_button(ui, 76, 194, 88, 30, "УПРАВЛЕНИЕ", color_warning(),
                ACTION_OPEN_CONTROLS, 0u, false);
}

static void draw_sparkline(gno_context_t *graphics)
{
    static const int16_t points[8][2] = {
        {42, 156}, {62, 150}, {82, 153}, {102, 139},
        {122, 144}, {142, 129}, {162, 136}, {194, 119}
    };
    gno_draw_hline(graphics, 38, 164, 164, color_line());
    for (uint8_t i = 1u; i < 8u; ++i) {
        gno_draw_line(graphics,
                      points[i - 1u][0], points[i - 1u][1],
                      points[i][0], points[i][1],
                      color_info());
    }
}

static void draw_metric_detail(watch_ui_t *ui)
{
    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    const watch_ui_metric_t *metric =
        &device->metrics[ui->selected_metric];
    draw_header(ui, metric->compact_label, true, color_info());

    if (ui->selected_metric == 2u) {
        text_center(ui->graphics, 120, 77, device->mode, 4u, color_text());
    } else {
        draw_metric_value(ui->graphics, 120, 73, metric, 4u);
    }
    draw_quality(ui->graphics, 82, 111, metric->quality);

    draw_card(ui->graphics, 28, 130, 184, 49, color_line());
    watch_draw_text(ui->graphics, 39, 138, "ТЕСТОВЫЙ ТРЕНД", 1u, color_muted());
    if (ui->selected_metric != 2u) {
        draw_sparkline(ui->graphics);
    } else {
        text_center(ui->graphics, 120, 154,
                    "ПОДТВЕРЖДЕНО", 1u, color_accent());
    }

    draw_button(ui, 76, 188, 88, 32, "УПРАВЛЕНИЕ", color_warning(),
                ACTION_OPEN_CONTROLS, 0u, false);
}

static void draw_control_list(watch_ui_t *ui)
{
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        draw_strict_control_list(ui);
        return;
    }
    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    draw_header(ui, "УПРАВЛЕНИЕ", true, color_warning());

    draw_card(ui->graphics, 28, 56, 184, 54, color_warning());
    watch_draw_text(ui->graphics, 40, 66, "РЕЖИМ", 2u, color_text());
    watch_draw_text(ui->graphics, 40, 91, device->mode, 1u, color_muted());
    watch_draw_text(ui->graphics, 137, 91, "С ЗАЩИТОЙ", 1u, color_warning());
    add_hit(ui, 24, 51, 192, 63, ACTION_OPEN_COMMAND, 0u, false);

    draw_card(ui->graphics, 28, 122, 184, 54, color_danger());
    watch_draw_text(ui->graphics, 40, 132, "ОСТАНОВ", 2u, color_text());
    watch_draw_text(ui->graphics, 40, 157, "ТОЛЬКО ЗАПРОС", 1u,
                    color_danger());
    watch_draw_text(ui->graphics, 154, 157, "КРИТИЧНО", 1u,
                    color_danger());
    add_hit(ui, 24, 117, 192, 63, ACTION_OPEN_CRITICAL, 0u, false);

    text_center(ui->graphics, 120, 194,
                "ЖЕЛАЕМОЕ НЕ ПОДТВЕРЖДЕНО", 1u, color_muted());
}

static void draw_hold_progress(watch_ui_t *ui, gno_color_t color)
{
    int width = ((int)ui->hold_progress * 168) / 100;
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        width = ((int)ui->hold_progress * 120) / 100;
        gno_fill_rect(ui->graphics, 60, 220, 120, 4, color_line());
        if (width > 0) {
            gno_fill_rect(ui->graphics, 60, 220, width, 4, color_text());
        }
        return;
    }
    gno_fill_rect(ui->graphics, 36, 203, 168, 7, color_surface_high());
    if (width > 0) {
        gno_fill_rect(ui->graphics, 36, 203, width, 7, color);
    }
}

static void draw_strict_command_status(watch_ui_t *ui)
{
    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    draw_header(ui, "ЗАЩИЩЕНО", true, color_text());

    if (ui->command_phase == WATCH_UI_COMMAND_CONFIRM) {
        watch_strict_draw_icon(ui->graphics, 27, 68,
                               WATCH_STRICT_ICON_LOCK, color_text());
        watch_draw_text(ui->graphics, 62, 71, "СМЕНА РЕЖИМА", 2u,
                        color_muted());
        watch_draw_text(ui->graphics, 62, 92, device->mode, 3u,
                        color_text());
        watch_strict_draw_icon(ui->graphics, 170, 83,
                               WATCH_STRICT_ICON_CARET_RIGHT, color_text());
        watch_draw_text(ui->graphics, 199, 92, "MAN", 2u, color_text());
        gno_draw_hline(ui->graphics, 26, 119, 188, color_line());
        text_center(ui->graphics, 120, 132,
                    "АВТОМАТИКА ПЕРЕСТАНЕТ", 1u, color_text());
        text_center(ui->graphics, 120, 149,
                    "УПРАВЛЯТЬ ПРИВОДОМ", 1u, color_muted());
        draw_button(ui, 52, 174, 136, 44, "УДЕРЖИВАТЬ",
                    color_text(), ACTION_SUBMIT_COMMAND, 0u, true);
        draw_hold_progress(ui, color_text());
        return;
    }

    if (ui->command_phase == WATCH_UI_COMMAND_SENDING) {
        watch_strict_draw_icon(ui->graphics, 104, 69,
                               WATCH_STRICT_ICON_SPINNER, color_text());
        text_center(ui->graphics, 120, 109, "ОТПРАВКА", 3u, color_text());
        text_center(ui->graphics, 120, 139, "КОМАНДА 1843", 1u,
                    color_muted());
        text_center(ui->graphics, 120, 158, "ЖДЕМ ОТВЕТ", 1u,
                    color_text());
        gno_draw_rect(ui->graphics, 52, 188, 136, 8, color_line());
        gno_fill_rect(ui->graphics, 55, 191, 80, 2, color_text());
        return;
    }

    const char *title = "ПОДТВЕРЖДЕНО";
    const char *detail = "РЕЖИМ MANUAL";
    watch_strict_icon_t icon = WATCH_STRICT_ICON_CHECK;
    if (ui->command_phase == WATCH_UI_COMMAND_REJECTED) {
        title = "ОТКЛОНЕНО";
        detail = "СОСТОЯНИЕ ПРЕЖНЕЕ";
        icon = WATCH_STRICT_ICON_ERROR;
    } else if (ui->command_phase == WATCH_UI_COMMAND_TIMEOUT) {
        title = "НЕТ ОТВЕТА";
        detail = "НЕТ ПОДТВЕРЖДЕНИЯ";
        icon = WATCH_STRICT_ICON_WARNING;
    }
    watch_strict_draw_icon(ui->graphics, 104, 69, icon, color_text());
    text_center(ui->graphics, 120, 108, title, 3u, color_text());
    text_center(ui->graphics, 120, 139, detail, 1u, color_text());
    text_center(ui->graphics, 120, 157, "РЕВИЗИЯ 1843", 1u,
                color_muted());
    draw_button(ui, 52, 174, 136, 44, "СЛЕДУЮЩИЙ ТЕСТ",
                color_text(), ACTION_NEXT_COMMAND_CASE, 0u, false);
}

static void draw_command_status(watch_ui_t *ui)
{
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        draw_strict_command_status(ui);
        return;
    }
    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    draw_header(ui, "КОМАНДА", true, color_warning());

    if (ui->command_phase == WATCH_UI_COMMAND_CONFIRM) {
        text_center(ui->graphics, 120, 57, "ЗАДАТЬ РЕЖИМ", 2u,
                    color_text());
        text_center(ui->graphics, 120, 84, device->mode, 2u,
                    color_muted());
        text_center(ui->graphics, 120, 104, "В MANUAL", 2u,
                    color_warning());
        draw_card(ui->graphics, 31, 130, 178, 43, color_line());
        text_center(ui->graphics, 120, 138, "АВТОМАТИКА ПЕРЕСТАНЕТ", 1u,
                    color_text());
        text_center(ui->graphics, 120, 152, "УПРАВЛЯТЬ ПРИВОДОМ", 1u,
                    color_muted());
        draw_button(ui, 36, 181, 168, 36, "УДЕРЖИВАТЬ",
                    color_warning(), ACTION_SUBMIT_COMMAND, 0u, true);
        draw_hold_progress(ui, color_warning());
        return;
    }

    if (ui->command_phase == WATCH_UI_COMMAND_SENDING) {
        text_center(ui->graphics, 120, 73, "ОТПРАВКА", 3u, color_warning());
        text_center(ui->graphics, 120, 112, "КОМАНДА ID 1843", 1u,
                    color_muted());
        text_center(ui->graphics, 120, 132, "ЖДЕМ ОТВЕТ", 1u,
                    color_text());
        gno_draw_rect(ui->graphics, 62, 159, 116, 12, color_line());
        gno_fill_rect(ui->graphics, 65, 162, 74, 6, color_warning());
        return;
    }

    const char *title = "ПОДТВЕРЖДЕНО";
    const char *detail = "РЕЖИМ MANUAL";
    gno_color_t status_color = color_accent();
    if (ui->command_phase == WATCH_UI_COMMAND_REJECTED) {
        title = "ОТКЛОНЕНО";
        detail = "СОСТОЯНИЕ ПРЕЖНЕЕ";
        status_color = color_danger();
    } else if (ui->command_phase == WATCH_UI_COMMAND_TIMEOUT) {
        title = "НЕТ ОТВЕТА";
        detail = "НЕТ ПОДТВЕРЖДЕНИЯ";
        status_color = color_warning();
    }
    text_center(ui->graphics, 120, 67, title, 3u, status_color);
    text_center(ui->graphics, 120, 111, detail, 1u, color_text());
    text_center(ui->graphics, 120, 132, "РЕВИЗИЯ 1843", 1u,
                color_muted());
    draw_button(ui, 36, 178, 168, 38, "СЛЕДУЮЩИЙ ТЕСТ",
                status_color, ACTION_NEXT_COMMAND_CASE, 0u, false);
}

static void draw_strict_critical_request(watch_ui_t *ui)
{
    draw_header(ui, "КРИТИЧЕСКАЯ", true, color_text());

    if (ui->critical_phase == WATCH_UI_CRITICAL_CONFIRM) {
        watch_strict_draw_icon(ui->graphics, 104, 67,
                               WATCH_STRICT_ICON_STOP, color_text());
        text_center(ui->graphics, 120, 105, "ОСТАНОВИТЬ?", 3u,
                    color_text());
        gno_draw_hline(ui->graphics, 26, 132, 188, color_line());
        text_center(ui->graphics, 120, 143,
                    "КОНТУР ПОТЕРЯЕТ ДАВЛЕНИЕ", 1u, color_text());
        text_center(ui->graphics, 120, 160,
                    "ТОЛЬКО ВНЕШНЕЕ РЕШЕНИЕ", 1u, color_muted());
        draw_button(ui, 52, 174, 136, 44, "УДЕРЖИВАТЬ",
                    color_text(), ACTION_SUBMIT_CRITICAL, 0u, true);
        draw_hold_progress(ui, color_text());
        return;
    }

    if (ui->critical_phase == WATCH_UI_CRITICAL_WAITING) {
        watch_strict_draw_icon(ui->graphics, 104, 68,
                               WATCH_STRICT_ICON_SPINNER, color_text());
        text_center(ui->graphics, 120, 108, "ЗАПРОС ОТПРАВЛЕН", 2u,
                    color_text());
        text_center(ui->graphics, 120, 139, "ЖДЕМ ВНЕШНЕЕ РЕШЕНИЕ", 1u,
                    color_muted());
        text_center(ui->graphics, 120, 166,
                    "ЛОКАЛЬНО НЕ ВЫПОЛНЯЕТСЯ", 1u, color_text());
        return;
    }

    const bool denied = ui->critical_phase == WATCH_UI_CRITICAL_DENIED;
    watch_strict_draw_icon(ui->graphics, 104, 68,
                           denied ? WATCH_STRICT_ICON_ERROR :
                           WATCH_STRICT_ICON_CHECK,
                           color_text());
    text_center(ui->graphics, 120, 108,
                denied ? "ОТКАЗАНО" : "ОДОБРЕНО", 3u, color_text());
    text_center(ui->graphics, 120, 139,
                denied ? "ЗАПРОС ЗАКРЫТ" : "НЕ ВЫПОЛНЕНО",
                1u, color_text());
    text_center(ui->graphics, 120, 157,
                "ОДОБРЕНИЕ НЕ ДЕЙСТВИЕ", 1u, color_muted());
    draw_button(ui, 52, 174, 136, 44, "СЛЕДУЮЩИЙ ТЕСТ",
                color_text(), ACTION_NEXT_CRITICAL_CASE, 0u, false);
}

static void draw_critical_request(watch_ui_t *ui)
{
    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        draw_strict_critical_request(ui);
        return;
    }
    draw_header(ui, "КРИТИЧНО", true, color_danger());

    if (ui->critical_phase == WATCH_UI_CRITICAL_CONFIRM) {
        text_center(ui->graphics, 120, 57, "ОСТАНОВИТЬ?", 3u,
                    color_danger());
        draw_card(ui->graphics, 31, 104, 178, 57, color_danger());
        text_center(ui->graphics, 120, 115, "КОНТУР ПОТЕРЯЕТ", 1u,
                    color_text());
        text_center(ui->graphics, 120, 134, "ДАВЛЕНИЕ", 2u,
                    color_warning());
        draw_button(ui, 36, 178, 168, 39, "УДЕРЖИВАТЬ",
                    color_danger(), ACTION_SUBMIT_CRITICAL, 0u, true);
        draw_hold_progress(ui, color_danger());
        return;
    }

    if (ui->critical_phase == WATCH_UI_CRITICAL_WAITING) {
        text_center(ui->graphics, 120, 66, "ЗАПРОС ОТПРАВЛЕН", 2u,
                    color_warning());
        text_center(ui->graphics, 120, 97, "ЖДЕМ ВНЕШНЕЕ", 1u,
                    color_text());
        text_center(ui->graphics, 120, 115, "РЕШЕНИЕ", 2u,
                    color_text());
        text_center(ui->graphics, 120, 151, "ЛОКАЛЬНО НЕ ВЫПОЛНЯЕТСЯ", 1u,
                    color_danger());
        return;
    }

    const char *title = "ОДОБРЕНО";
    const char *detail = "НЕ ВЫПОЛНЕНО";
    gno_color_t status_color = color_accent();
    if (ui->critical_phase == WATCH_UI_CRITICAL_DENIED) {
        title = "ОТКАЗАНО";
        detail = "ЗАПРОС ЗАКРЫТ";
        status_color = color_danger();
    }
    text_center(ui->graphics, 120, 65, title, 3u, status_color);
    text_center(ui->graphics, 120, 108, detail, 2u, color_danger());
    text_center(ui->graphics, 120, 139,
                "ОДОБРЕНИЕ НЕ ДЕЙСТВИЕ", 1u, color_muted());
    draw_button(ui, 36, 178, 168, 38, "СЛЕДУЮЩИЙ ТЕСТ",
                status_color, ACTION_NEXT_CRITICAL_CASE, 0u, false);
}

static gno_color_t event_color(watch_ui_event_severity_t severity)
{
    if (severity == WATCH_UI_EVENT_BLOCKING) {
        return color_danger();
    }
    if (severity == WATCH_UI_EVENT_IMPORTANT) {
        return color_warning();
    }
    return color_info();
}

static const char *event_severity_label(
    watch_ui_event_severity_t severity)
{
    if (severity == WATCH_UI_EVENT_BLOCKING) {
        return "БЛОК";
    }
    if (severity == WATCH_UI_EVENT_IMPORTANT) {
        return "ВАЖНО";
    }
    return "ИНФО";
}

static void draw_event_journal(watch_ui_t *ui)
{
    draw_header(ui, "СОБЫТИЯ", true, color_warning());

    for (uint8_t i = 0u; i < ui->model.event_count; ++i) {
        const watch_ui_event_t *event = &ui->model.events[i];
        int y = (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
                64 + ((int)i * 55) : 49 + ((int)i * 48);
        gno_color_t color = event_color(event->severity);
        if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
            watch_strict_icon_t icon = WATCH_STRICT_ICON_CHECK;
            if (event->severity == WATCH_UI_EVENT_BLOCKING) {
                icon = WATCH_STRICT_ICON_ERROR;
            } else if (event->severity == WATCH_UI_EVENT_IMPORTANT) {
                icon = WATCH_STRICT_ICON_WARNING;
            }
            watch_strict_draw_icon(ui->graphics, 28, y + 10,
                                   icon, color_text());
            watch_draw_text(ui->graphics, 64, y + 8,
                            event->title, 1u, color_text());
            watch_draw_text(ui->graphics, 64, y + 31,
                            event_severity_label(event->severity),
                            1u, color_muted());
            watch_strict_draw_icon(ui->graphics, 181, y + 10,
                                   WATCH_STRICT_ICON_CARET_RIGHT,
                                   color_text());
            gno_draw_hline(ui->graphics, 26, y + 54, 188, color_line());
            add_hit(ui, 24, y, 192, 55,
                    ACTION_OPEN_EVENT, i, false);
            continue;
        }
        draw_card(ui->graphics, 28, y, 184, 40, color);
        gno_fill_rect(ui->graphics, 38, y + 8, 6, 24, color);
        watch_draw_text(ui->graphics, 54, y + 7,
                        event->title, 2u, color_text());
        watch_draw_text(ui->graphics, 54, y + 27,
                        event_severity_label(event->severity), 1u, color);
        if (event->acknowledged) {
            watch_draw_text(ui->graphics, 174, y + 27,
                            "ПРИН", 1u, color_muted());
        }
        add_hit(ui, 24, y - 2, 192, 44,
                ACTION_OPEN_EVENT, i, false);
    }
}

static void draw_event_detail(watch_ui_t *ui)
{
    const watch_ui_event_t *event =
        &ui->model.events[ui->selected_event];
    gno_color_t color = event_color(event->severity);
    draw_header(ui, event_severity_label(event->severity), true, color);

    if (active_skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
        watch_strict_draw_icon(ui->graphics, 104, 66,
                               event->severity == WATCH_UI_EVENT_BLOCKING ?
                               WATCH_STRICT_ICON_ERROR :
                               WATCH_STRICT_ICON_WARNING,
                               color_text());
        text_center(ui->graphics, 120, 101,
                    event->source_id, 1u, color_muted());
        text_center(ui->graphics, 120, 119,
                    event->title, 2u, color_text());
        gno_draw_hline(ui->graphics, 26, 146, 188, color_line());
        text_center(ui->graphics, 120, 153,
                    event->detail_line_1, 1u, color_text());
        text_center(ui->graphics, 120, 168,
                    event->detail_line_2, 1u, color_muted());
        draw_button(ui, 52, 174, 136, 44,
                    event->acknowledged ? "ЗАКРЫТЬ" : "ПОДТВЕРДИТЬ",
                    color_text(), ACTION_ACK_EVENT, 0u, false);
        return;
    }
    text_center(ui->graphics, 120, 61,
                event->source_id, 1u, color_muted());
    text_center(ui->graphics, 120, 83,
                event->title, 2u, color);
    draw_card(ui->graphics, 31, 112, 178, 54, color);
    text_center(ui->graphics, 120, 122,
                event->detail_line_1, 1u, color_text());
    text_center(ui->graphics, 120, 142,
                event->detail_line_2, 1u, color_text());

    draw_button(ui, 36, 179, 168, 38,
                event->acknowledged ? "ЗАКРЫТЬ" : "ПОДТВЕРДИТЬ",
                color, ACTION_ACK_EVENT, 0u, false);
}

static void draw_diagnostic(watch_ui_t *ui, watch_time_t time)
{
    static const int16_t marks[12][4] = {
        {120, 13, 120, 25}, {174, 28, 168, 39},
        {212, 66, 201, 73}, {227, 120, 215, 120},
        {212, 174, 201, 167}, {174, 212, 168, 201},
        {120, 227, 120, 215}, {66, 212, 73, 201},
        {28, 174, 39, 167}, {13, 120, 25, 120},
        {28, 66, 39, 73}, {66, 28, 73, 39}
    };

    for (uint8_t i = 0u; i < 12u; ++i) {
        gno_draw_line(ui->graphics,
                      marks[i][0], marks[i][1],
                      marks[i][2], marks[i][3],
                      color_muted());
    }
    text_center(ui->graphics, 120, 37, "ДИАГНОСТИКА", 1u, color_warning());
    draw_home_time(ui, time);
    draw_home_progress(ui, time);
    text_center(ui->graphics, 120, 138, "TOUCH B6 / GUI НОРМА", 1u,
                color_accent());
    text_center(ui->graphics, 120, 151,
                (ui->skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
                "СКИН: СТРОГИЙ" : "СКИН: ЦВЕТНОЙ",
                1u,
                color_muted());
    draw_button(ui, 46, 164, 148, 28, "СМЕНИТЬ СКИН", color_warning(),
                ACTION_TOGGLE_SKIN, 0u, false);
    draw_button(ui, 76, 198, 88, 26, "НАЗАД", color_info(),
                ACTION_BACK, 0u, false);
}

static void draw_media_marquee(gno_context_t *graphics,
                               int x, int y, int width,
                               const char *text, uint8_t scale,
                               gno_color_t color, uint32_t phase)
{
    int text_width = skin_text_width(text, scale);
    gno_fill_rect(graphics, x, y, width, 22, color_background());
    if (text_width <= width) {
        watch_draw_text(graphics, x, y, text, scale, color);
        return;
    }

    int cycle = text_width + 20;
    int offset = (int)((phase * 2u) % (uint32_t)cycle);
    if (gno_push_clip(graphics,
                      (gno_rect_t) { x, y, width, 22 })) {
        watch_draw_text(graphics, x - offset, y, text, scale, color);
        watch_draw_text(graphics, x - offset + cycle, y,
                        text, scale, color);
        (void)gno_pop_state(graphics);
    }
}

static void draw_media_texts(watch_ui_t *ui, watch_time_t time)
{
    const watch_ui_media_status_t *media = &ui->media;
    uint32_t phase = time_ticks(time) / 2u;
    draw_media_marquee(ui->graphics, 124, 72, 78,
                       media->track[0] ? media->track : "нет трека",
                       2u, color_text(), phase);
    draw_media_marquee(ui->graphics, 124, 103, 78,
                       media->artist[0] ? media->artist : "Gadgetbridge",
                       1u, color_muted(), phase);
}

static void draw_media_screen(watch_ui_t *ui, watch_time_t time)
{
    const watch_ui_media_status_t *media = &ui->media;
    char time_text[6] = "00:00";
    uint32_t position = media->position_s;
    uint32_t minutes = position / 60u;
    uint32_t seconds = position % 60u;
    time_text[0] = (char)('0' + ((minutes / 10u) % 10u));
    time_text[1] = (char)('0' + (minutes % 10u));
    time_text[3] = (char)('0' + (seconds / 10u));
    time_text[4] = (char)('0' + (seconds % 10u));
    draw_header(ui, "МУЗЫКА", true, color_text());
    gno_draw_rect(ui->graphics, 28, 55, 184, 92, color_line());
    if (ui->artwork.valid && ui->artwork.pixels != NULL &&
        ui->artwork.width == 80u && ui->artwork.height == 80u) {
        gno_bitmap_t artwork = {
            .width = ui->artwork.width,
            .height = ui->artwork.height,
            .row_stride_bytes = 160u,
            .format = GNO_PIXELFORMAT_RGB565,
            .pixels = ui->artwork.pixels
        };
        (void)gno_draw_bitmap(ui->graphics, 34, 61, &artwork);
    } else {
        gno_fill_rect(ui->graphics, 34, 61, 80, 80, color_info());
        watch_draw_text(ui->graphics, 61, 82,
                        media->state == 1u ? ">" : "||", 4u,
                        color_text());
    }
    draw_media_texts(ui, time);
    watch_draw_text(ui->graphics, 32, 162, time_text, 1u, color_muted());
    gno_draw_rect(ui->graphics, 32, 181, 176, 6, color_line());
    uint16_t progress = media->duration_s == 0u ? 0u :
        (uint16_t)((uint32_t)172u * media->position_s / media->duration_s);
    if (progress > 172u) progress = 172u;
    gno_fill_rect(ui->graphics, 34, 183, progress, 2, color_info());
    gno_draw_rect(ui->graphics, 28, 199, 52, 30, color_info());
    gno_draw_rect(ui->graphics, 94, 199, 52, 30, color_accent());
    gno_draw_rect(ui->graphics, 160, 199, 52, 30, color_info());
    add_hit(ui, 28, 199, 52, 30, ACTION_MEDIA_PREVIOUS, 3u, false);
    add_hit(ui, 94, 199, 52, 30, ACTION_MEDIA_PLAY_PAUSE, 0u, false);
    add_hit(ui, 160, 199, 52, 30, ACTION_MEDIA_NEXT, 4u, false);
    watch_draw_text(ui->graphics, 45, 207, "|<", 1u, color_text());
    watch_draw_text(ui->graphics, 111, 207, media->state == 1u ? "||" : ">",
                    1u, color_text());
    watch_draw_text(ui->graphics, 177, 207, ">|", 1u, color_text());
}

static bool render_screen(watch_ui_t *ui, watch_time_t time)
{
    reset_hits(ui);
    gno_clear(ui->graphics, color_background());

    switch (ui->screen) {
        case WATCH_UI_SCREEN_HOME_CONTEXT:
            if (ui->skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
                draw_strict_home(ui, time);
            } else {
                draw_home(ui, time);
            }
            break;
        case WATCH_UI_SCREEN_DEVICE_LIST:
            draw_device_list(ui);
            break;
        case WATCH_UI_SCREEN_DEVICE_OVERVIEW:
            draw_overview(ui);
            break;
        case WATCH_UI_SCREEN_PARAMETER_LIST:
            draw_parameter_list(ui);
            break;
        case WATCH_UI_SCREEN_METRIC_DETAIL:
            draw_metric_detail(ui);
            break;
        case WATCH_UI_SCREEN_CONTROL_LIST:
            draw_control_list(ui);
            break;
        case WATCH_UI_SCREEN_COMMAND_STATUS:
            draw_command_status(ui);
            break;
        case WATCH_UI_SCREEN_CRITICAL_REQUEST:
            draw_critical_request(ui);
            break;
        case WATCH_UI_SCREEN_EVENT_JOURNAL:
            draw_event_journal(ui);
            break;
        case WATCH_UI_SCREEN_EVENT_DETAIL:
            draw_event_detail(ui);
            break;
        case WATCH_UI_SCREEN_DIAGNOSTIC:
            draw_diagnostic(ui, time);
            break;
        case WATCH_UI_SCREEN_MEDIA:
            draw_media_screen(ui, time);
            break;
        default:
            return false;
    }

    if (gno_has_error(ui->graphics)) {
        gno_reset_error(ui->graphics);
        return false;
    }
    ui->displayed_time = time;
    ui->needs_redraw = false;
    return true;
}

static void navigate_to(watch_ui_t *ui, watch_ui_screen_t screen)
{
    if (ui->screen == screen) {
        return;
    }

    if (ui->navigation_depth < WATCH_UI_NAV_DEPTH) {
        ui->navigation[ui->navigation_depth++] = ui->screen;
    } else {
        for (uint8_t i = 1u; i < WATCH_UI_NAV_DEPTH; ++i) {
            ui->navigation[i - 1u] = ui->navigation[i];
        }
        ui->navigation[WATCH_UI_NAV_DEPTH - 1u] = ui->screen;
    }
    ui->screen = screen;
    ui->pressed_hit = -1;
    ui->touch_down = false;
    ui->hold_active = false;
    ui->hold_progress = 0u;
    ui->needs_redraw = true;
}

static void navigate_back(watch_ui_t *ui)
{
    if (ui->navigation_depth == 0u) {
        ui->screen = WATCH_UI_SCREEN_HOME_CONTEXT;
    } else {
        ui->navigation_depth--;
        ui->screen = ui->navigation[ui->navigation_depth];
    }
    ui->pressed_hit = -1;
    ui->touch_down = false;
    ui->hold_active = false;
    ui->hold_progress = 0u;
    ui->needs_redraw = true;
}

bool watch_ui_present_event(watch_ui_t *ui, uint8_t event_index)
{
    if (ui == NULL || !ui->initialized ||
        event_index >= ui->model.event_count) {
        return false;
    }

    ui->selected_event = event_index;
    ui->needs_redraw = true;
    navigate_to(ui, WATCH_UI_SCREEN_EVENT_DETAIL);
    return watch_ui_update(ui, watch_clock_get()) &&
           watch_ui_debug_validate(ui);
}

static void dispatch_action(watch_ui_t *ui,
                            ui_action_t action,
                            uint8_t argument,
                            watch_time_t time)
{
    switch (action) {
        case ACTION_BACK:
            navigate_back(ui);
            break;
        case ACTION_OPEN_DEVICES:
            navigate_to(ui, WATCH_UI_SCREEN_DEVICE_LIST);
            break;
        case ACTION_OPEN_OVERVIEW:
            if (argument < ui->model.device_count) {
                ui->selected_device = argument;
                ui->selected_metric = 0u;
            }
            navigate_to(ui, WATCH_UI_SCREEN_DEVICE_OVERVIEW);
            break;
        case ACTION_OPEN_PARAMETERS:
            navigate_to(ui, WATCH_UI_SCREEN_PARAMETER_LIST);
            break;
        case ACTION_OPEN_METRIC:
            ui->selected_metric = argument;
            navigate_to(ui, WATCH_UI_SCREEN_METRIC_DETAIL);
            break;
        case ACTION_OPEN_CONTROLS:
            navigate_to(ui, WATCH_UI_SCREEN_CONTROL_LIST);
            break;
        case ACTION_OPEN_COMMAND:
            ui->command_phase = WATCH_UI_COMMAND_CONFIRM;
            navigate_to(ui, WATCH_UI_SCREEN_COMMAND_STATUS);
            break;
        case ACTION_SUBMIT_COMMAND:
            ui->command_phase = WATCH_UI_COMMAND_SENDING;
            ui->phase_started_at = time_ticks(time);
            ui->needs_redraw = true;
            break;
        case ACTION_NEXT_COMMAND_CASE:
            ui->command_demo_outcome =
                (uint8_t)((ui->command_demo_outcome + 1u) % 3u);
            ui->model.devices[ui->selected_device].mode = "AUTO";
            ui->model.source_revision = 1842u;
            ui->command_phase = WATCH_UI_COMMAND_CONFIRM;
            ui->needs_redraw = true;
            break;
        case ACTION_OPEN_CRITICAL:
            ui->critical_phase = WATCH_UI_CRITICAL_CONFIRM;
            navigate_to(ui, WATCH_UI_SCREEN_CRITICAL_REQUEST);
            break;
        case ACTION_SUBMIT_CRITICAL:
            ui->critical_phase = WATCH_UI_CRITICAL_WAITING;
            ui->phase_started_at = time_ticks(time);
            ui->needs_redraw = true;
            break;
        case ACTION_NEXT_CRITICAL_CASE:
            ui->critical_demo_outcome ^= 1u;
            ui->critical_phase = WATCH_UI_CRITICAL_CONFIRM;
            ui->needs_redraw = true;
            break;
        case ACTION_OPEN_EVENTS:
            navigate_to(ui, WATCH_UI_SCREEN_EVENT_JOURNAL);
            break;
        case ACTION_OPEN_EVENT:
            (void)watch_ui_present_event(ui, argument);
            break;
        case ACTION_ACK_EVENT:
            ui->model.events[ui->selected_event].acknowledged = true;
            navigate_back(ui);
            break;
        case ACTION_OPEN_DIAGNOSTIC:
            navigate_to(ui, WATCH_UI_SCREEN_DIAGNOSTIC);
            break;
        case ACTION_OPEN_MEDIA:
            navigate_to(ui, WATCH_UI_SCREEN_MEDIA);
            break;
        case ACTION_MEDIA_PREVIOUS:
        case ACTION_MEDIA_PLAY_PAUSE:
        case ACTION_MEDIA_NEXT:
            ui->media_command = (uint8_t)argument;
            ui->media_command_pending = true;
            ui->needs_redraw = true;
            break;
        case ACTION_TOGGLE_SKIN:
            ui->skin =
                (ui->skin == WATCH_UI_SKIN_STRICT_CONTEXT) ?
                WATCH_UI_SKIN_COLOR_CONTEXT :
                WATCH_UI_SKIN_STRICT_CONTEXT;
            active_skin = ui->skin;
            ui->needs_redraw = true;
            break;
        case ACTION_NONE:
        default:
            break;
    }
}

static int8_t find_hit(const watch_ui_t *ui, uint16_t x, uint16_t y)
{
    for (uint8_t i = 0u; i < ui->hit_count; ++i) {
        if (point_in_rect(x, y, &ui->hits[i])) {
            return (int8_t)i;
        }
    }
    return -1;
}

bool watch_ui_init(watch_ui_t *ui, gno_context_t *graphics)
{
    if (ui == NULL || graphics == NULL) {
        return false;
    }

    ui->graphics = graphics;
    watch_ui_demo_model_init(&ui->model);
    ui->screen = WATCH_UI_SCREEN_HOME_CONTEXT;
    ui->navigation_depth = 0u;
    ui->selected_device = 0u;
    ui->selected_metric = 0u;
    ui->selected_event = 0u;
    ui->hit_count = 0u;
    ui->pressed_hit = -1;
    ui->touch_down = false;
    ui->hold_active = false;
    ui->hold_started_at = 0u;
    ui->hold_progress = 0u;
    ui->command_phase = WATCH_UI_COMMAND_IDLE;
    ui->critical_phase = WATCH_UI_CRITICAL_IDLE;
    ui->skin = WATCH_UI_SKIN_STRICT_CONTEXT;
    ui->command_demo_outcome = 0u;
    ui->critical_demo_outcome = 0u;
    ui->phone = (watch_ui_phone_status_t) {
        .state = WATCH_UI_PHONE_OFF,
        .connections = 0u,
        .received_packets = 0u,
        .tx_notifications = 0u
    };
    ui->phase_started_at = 0u;
    ui->displayed_time = (watch_time_t) {
        .hour = 0xFFu,
        .minute = 0xFFu,
        .second = 0xFFu,
        .subsecond = 0xFFu
    };
    ui->initialized = true;
    ui->needs_redraw = true;
    return true;
}

bool watch_ui_update(watch_ui_t *ui, watch_time_t time)
{
    if (ui == NULL || !ui->initialized) {
        return false;
    }

    active_skin = ui->skin;
    uint32_t now = time_ticks(time);
    if (ui->command_phase == WATCH_UI_COMMAND_SENDING &&
        elapsed_ticks(ui->phase_started_at, now) >= COMMAND_RESULT_TICKS) {
        if (ui->command_demo_outcome == 0u) {
            ui->command_phase = WATCH_UI_COMMAND_SUCCESS;
            ui->model.devices[ui->selected_device].mode = "MANUAL";
            ui->model.source_revision++;
        } else if (ui->command_demo_outcome == 1u) {
            ui->command_phase = WATCH_UI_COMMAND_REJECTED;
        } else {
            ui->command_phase = WATCH_UI_COMMAND_TIMEOUT;
        }
        ui->needs_redraw = true;
    }

    if (ui->critical_phase == WATCH_UI_CRITICAL_WAITING &&
        elapsed_ticks(ui->phase_started_at, now) >= CRITICAL_RESULT_TICKS) {
        ui->critical_phase =
            (ui->critical_demo_outcome == 0u) ?
            WATCH_UI_CRITICAL_APPROVED :
            WATCH_UI_CRITICAL_DENIED;
        ui->needs_redraw = true;
    }

    if (ui->needs_redraw) {
        return render_screen(ui, time);
    }

    if (ui->screen == WATCH_UI_SCREEN_HOME_CONTEXT ||
        ui->screen == WATCH_UI_SCREEN_DIAGNOSTIC) {
        if (ui->skin == WATCH_UI_SKIN_STRICT_CONTEXT) {
            if (time.minute != ui->displayed_time.minute ||
                time.hour != ui->displayed_time.hour) {
                ui->needs_redraw = true;
                return render_screen(ui, time);
            }
            ui->displayed_time = time;
            return true;
        }
        if (time.minute != ui->displayed_time.minute ||
            time.hour != ui->displayed_time.hour) {
            draw_home_time(ui, time);
        }
        if (time.second != ui->displayed_time.second ||
            time.subsecond != ui->displayed_time.subsecond) {
            draw_home_progress(ui, time);
        }
        ui->displayed_time = time;
    }

    if (ui->screen == WATCH_UI_SCREEN_MEDIA &&
        (time_ticks(time) / 2u) != (time_ticks(ui->displayed_time) / 2u)) {
        draw_media_texts(ui, time);
        ui->displayed_time = time;
    }

    if (gno_has_error(ui->graphics)) {
        gno_reset_error(ui->graphics);
        return false;
    }
    return true;
}

bool watch_ui_process_touch(watch_ui_t *ui,
                            bool down,
                            uint16_t x,
                            uint16_t y,
                            watch_time_t time)
{
    if (ui == NULL || !ui->initialized) {
        return false;
    }

    uint32_t now = time_ticks(time);

    if (down && !ui->touch_down) {
        ui->pressed_hit = find_hit(ui, x, y);
        if (ui->pressed_hit >= 0) {
            watch_ui_hit_target_t *target =
                &ui->hits[(uint8_t)ui->pressed_hit];
            if (target->requires_hold) {
                ui->hold_active = true;
                ui->hold_started_at = now;
                ui->hold_progress = 0u;
                draw_hold_progress(ui,
                    (target->action == ACTION_SUBMIT_CRITICAL) ?
                    color_danger() : color_warning());
            }
        }
    } else if (down && ui->touch_down && ui->pressed_hit >= 0) {
        watch_ui_hit_target_t *target =
            &ui->hits[(uint8_t)ui->pressed_hit];
        if (!point_in_rect(x, y, target)) {
            ui->hold_active = false;
            ui->pressed_hit = -1;
            ui->needs_redraw = true;
        } else if (target->requires_hold && ui->hold_active) {
            uint32_t held = elapsed_ticks(ui->hold_started_at, now);
            uint8_t progress =
                (held >= HOLD_TICKS) ? 100u :
                (uint8_t)((held * 100u) / HOLD_TICKS);
            if (progress != ui->hold_progress) {
                ui->hold_progress = progress;
                draw_hold_progress(ui,
                    (target->action == ACTION_SUBMIT_CRITICAL) ?
                    color_danger() : color_warning());
            }
            if (held >= HOLD_TICKS) {
                ui_action_t action = (ui_action_t)target->action;
                uint8_t argument = target->argument;
                ui->hold_active = false;
                ui->pressed_hit = -1;
                dispatch_action(ui, action, argument, time);
            }
        }
    } else if (!down && ui->touch_down) {
        if (ui->pressed_hit >= 0) {
            watch_ui_hit_target_t target =
                ui->hits[(uint8_t)ui->pressed_hit];
            if (!target.requires_hold && point_in_rect(x, y, &target)) {
                dispatch_action(ui, (ui_action_t)target.action,
                                target.argument, time);
            } else if (target.requires_hold) {
                ui->needs_redraw = true;
            }
        }
        ui->pressed_hit = -1;
        ui->hold_active = false;
        ui->hold_progress = 0u;
    }

    ui->touch_down = down;
    return true;
}

watch_ui_screen_t watch_ui_get_screen(const watch_ui_t *ui)
{
    return (ui != NULL) ? ui->screen : WATCH_UI_SCREEN_HOME_CONTEXT;
}

watch_ui_skin_t watch_ui_get_skin(const watch_ui_t *ui)
{
    return (ui != NULL) ? ui->skin : WATCH_UI_SKIN_STRICT_CONTEXT;
}

bool watch_ui_set_skin(watch_ui_t *ui, watch_ui_skin_t skin)
{
    if (ui == NULL || !ui->initialized ||
        skin > WATCH_UI_SKIN_STRICT_CONTEXT) {
        return false;
    }
    ui->skin = skin;
    active_skin = skin;
    ui->needs_redraw = true;
    return watch_ui_update(ui, watch_clock_get());
}

bool watch_ui_set_phone_status(watch_ui_t *ui,
                               watch_ui_phone_status_t status)
{
    if (ui == NULL || !ui->initialized ||
        status.state > WATCH_UI_PHONE_ERROR) {
        return false;
    }

    if (ui->phone.state != status.state ||
        ui->phone.connections != status.connections ||
        ui->phone.received_packets != status.received_packets ||
        ui->phone.tx_notifications != status.tx_notifications) {
        ui->phone = status;
        if (ui->screen == WATCH_UI_SCREEN_HOME_CONTEXT ||
            ui->screen == WATCH_UI_SCREEN_DIAGNOSTIC) {
            ui->needs_redraw = true;
        }
    }
    return true;
}

bool watch_ui_set_media_status(watch_ui_t *ui,
                               const watch_ui_media_status_t *status)
{
    if (ui == NULL || !ui->initialized || status == NULL) return false;
    if (ui->media.revision != status->revision) {
        ui->media.state = status->state;
        ui->media.volume = status->volume;
        ui->media.duration_s = status->duration_s;
        ui->media.position_s = status->position_s;
        ui->media.revision = status->revision;
        for (uint16_t i = 0u; i < sizeof(ui->media.artist); ++i) {
            ui->media.artist[i] = status->artist[i];
            if (status->artist[i] == '\0') break;
        }
        for (uint16_t i = 0u; i < sizeof(ui->media.track); ++i) {
            ui->media.track[i] = status->track[i];
            if (status->track[i] == '\0') break;
        }
        if (ui->screen == WATCH_UI_SCREEN_MEDIA) ui->needs_redraw = true;
    }
    return true;
}

bool watch_ui_set_artwork_status(watch_ui_t *ui,
                                 watch_ui_artwork_status_t status)
{
    if (ui == NULL || !ui->initialized) return false;
    if (ui->artwork.revision != status.revision ||
        ui->artwork.valid != status.valid) {
        ui->artwork = status;
        if (ui->screen == WATCH_UI_SCREEN_MEDIA) ui->needs_redraw = true;
    }
    return true;
}

bool watch_ui_take_media_command(watch_ui_t *ui, uint8_t *command)
{
    if ((ui == NULL) || (command == NULL) || !ui->media_command_pending) {
        return false;
    }
    *command = ui->media_command;
    ui->media_command_pending = false;
    return true;
}

static watch_time_t add_test_ticks(watch_time_t time, uint16_t ticks)
{
    uint32_t total = (time_ticks(time) + ticks) % TICKS_PER_DAY;
    uint32_t seconds = total / WATCH_CLOCK_SUBSECOND_HZ;
    watch_time_t result;

    result.subsecond = (uint8_t)(total % WATCH_CLOCK_SUBSECOND_HZ);
    result.hour = (uint8_t)(seconds / 3600u);
    seconds %= 3600u;
    result.minute = (uint8_t)(seconds / 60u);
    result.second = (uint8_t)(seconds % 60u);
    return result;
}

bool watch_ui_debug_tap(watch_ui_t *ui, uint16_t x, uint16_t y)
{
    watch_time_t time = watch_clock_get();
    bool down_ok = watch_ui_process_touch(ui, true, x, y, time);
    bool up_ok = watch_ui_process_touch(ui, false, x, y, time);
    return down_ok && up_ok && watch_ui_update(ui, time) &&
           watch_ui_debug_validate(ui);
}

bool watch_ui_debug_hold(watch_ui_t *ui, uint16_t x, uint16_t y)
{
    watch_time_t start = watch_clock_get();
    watch_time_t complete = add_test_ticks(start, HOLD_TICKS);
    bool down_ok = watch_ui_process_touch(ui, true, x, y, start);
    bool hold_ok = watch_ui_process_touch(ui, true, x, y, complete);
    bool up_ok = watch_ui_process_touch(ui, false, x, y, complete);
    /*
     * The synthetic completion timestamp is ahead of the hardware RTC.
     * Rebase a newly started lifecycle phase to the real clock so the normal
     * main loop cannot interpret it as a nearly-one-day elapsed interval.
     */
    if (ui->command_phase == WATCH_UI_COMMAND_SENDING ||
        ui->critical_phase == WATCH_UI_CRITICAL_WAITING) {
        ui->phase_started_at = time_ticks(start);
    }
    return down_ok && hold_ok && up_ok && watch_ui_update(ui, start) &&
           watch_ui_debug_validate(ui);
}

bool watch_ui_debug_complete_phase(watch_ui_t *ui)
{
    if (ui == NULL || !ui->initialized) {
        return false;
    }

    watch_time_t now = watch_clock_get();
    if (ui->command_phase == WATCH_UI_COMMAND_SENDING) {
        ui->phase_started_at =
            (time_ticks(now) + TICKS_PER_DAY - COMMAND_RESULT_TICKS) %
            TICKS_PER_DAY;
    } else if (ui->critical_phase == WATCH_UI_CRITICAL_WAITING) {
        ui->phase_started_at =
            (time_ticks(now) + TICKS_PER_DAY - CRITICAL_RESULT_TICKS) %
            TICKS_PER_DAY;
    } else {
        return false;
    }
    return watch_ui_update(ui, now) && watch_ui_debug_validate(ui);
}

bool watch_ui_debug_validate(const watch_ui_t *ui)
{
    if (ui == NULL || !ui->initialized || ui->graphics == NULL ||
        ui->skin > WATCH_UI_SKIN_STRICT_CONTEXT ||
        ui->screen > WATCH_UI_SCREEN_DIAGNOSTIC ||
        ui->navigation_depth > WATCH_UI_NAV_DEPTH ||
        ui->hit_count > WATCH_UI_MAX_HIT_TARGETS ||
        ui->model.device_count == 0u || ui->model.device_count > 3u ||
        ui->model.event_count == 0u || ui->model.event_count > 3u ||
        ui->selected_device >= ui->model.device_count ||
        ui->selected_event >= ui->model.event_count) {
        return false;
    }

    const watch_ui_device_t *device =
        &ui->model.devices[ui->selected_device];
    if (device->metric_count == 0u || device->metric_count > 3u ||
        ui->selected_metric >= device->metric_count) {
        return false;
    }

    for (uint8_t i = 0u; i < ui->hit_count; ++i) {
        const watch_ui_hit_target_t *target = &ui->hits[i];
        if (target->w < MINIMUM_HIT_SIZE ||
            target->h < MINIMUM_HIT_SIZE ||
            target->x < 0 || target->y < 0 ||
            ((int32_t)target->x + target->w) > SCREEN_SIZE ||
            ((int32_t)target->y + target->h) > SCREEN_SIZE) {
            return false;
        }
    }
    return true;
}
