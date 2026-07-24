#include "watch/ui_demo_model.h"

#include <stddef.h>

static void set_metric(watch_ui_metric_t *metric,
                       const char *id,
                       const char *label,
                       const char *compact_label,
                       const char *unit,
                       int16_t value_tenths,
                       watch_ui_quality_t quality,
                       uint8_t priority)
{
    metric->id = id;
    metric->label = label;
    metric->compact_label = compact_label;
    metric->unit = unit;
    metric->value_tenths = value_tenths;
    metric->quality = quality;
    metric->priority = priority;
}

static void set_event(watch_ui_event_t *event,
                      const char *event_id,
                      const char *source_id,
                      const char *title,
                      const char *detail_line_1,
                      const char *detail_line_2,
                      watch_ui_event_severity_t severity,
                      bool acknowledged)
{
    event->event_id = event_id;
    event->source_id = source_id;
    event->title = title;
    event->detail_line_1 = detail_line_1;
    event->detail_line_2 = detail_line_2;
    event->severity = severity;
    event->active = true;
    event->acknowledged = acknowledged;
}

void watch_ui_demo_model_init(watch_ui_demo_model_t *model)
{
    if (model == NULL) {
        return;
    }

    model->device_count = 3u;
    model->source_revision = 1842u;

    watch_ui_device_t *pump = &model->devices[0];
    pump->source_id = "PUMP-2";
    pump->location = "КОНТУР 1";
    pump->status = "РАБОТАЕТ";
    pump->online = true;
    pump->metric_count = 3u;
    pump->mode = "AUTO";
    set_metric(&pump->metrics[0],
               "pressure.outlet", "ДАВЛЕНИЕ", "ДАВЛ", "bar",
               24, WATCH_UI_QUALITY_GOOD, 90u);
    set_metric(&pump->metrics[1],
               "motor.current", "ТОК ДВИГАТЕЛЯ", "ТОК", "A",
               48, WATCH_UI_QUALITY_STALE, 80u);
    set_metric(&pump->metrics[2],
               "drive.mode", "РЕЖИМ ПРИВОДА", "РЕЖИМ", "",
               0, WATCH_UI_QUALITY_GOOD, 70u);

    watch_ui_device_t *detector = &model->devices[1];
    detector->source_id = "ДЕТЕКТОР";
    detector->location = "ПЕРСОНАЛ";
    detector->status = "НЕТ СВЯЗИ";
    detector->online = false;
    detector->metric_count = 2u;
    detector->mode = "LOCAL";
    set_metric(&detector->metrics[0],
               "detector.signal", "СИГНАЛ", "СИГНАЛ", "%",
               0, WATCH_UI_QUALITY_OFFLINE, 90u);
    set_metric(&detector->metrics[1],
               "detector.battery", "БАТАРЕЯ", "БАТАРЕЯ", "%",
               720, WATCH_UI_QUALITY_STALE, 70u);

    watch_ui_device_t *local = &model->devices[2];
    local->source_id = "ЧАСЫ";
    local->location = "ЛОКАЛЬНО";
    local->status = "ГОТОВО";
    local->online = true;
    local->metric_count = 2u;
    local->mode = "NORMAL";
    set_metric(&local->metrics[0],
               "watch.battery", "БАТАРЕЯ", "БАТАРЕЯ", "%",
               780, WATCH_UI_QUALITY_GOOD, 90u);
    set_metric(&local->metrics[1],
               "watch.temperature", "ТЕМПЕРАТУРА", "ТЕМП", "C",
               241, WATCH_UI_QUALITY_UNCERTAIN, 60u);

    model->event_count = 3u;
    set_event(&model->events[0],
              "pump.low-pressure", "PUMP-2", "НИЗКОЕ ДАВЛЕНИЕ",
              "ВЫХОД НИЖЕ", "БЕЗОПАСНОЙ ЗОНЫ",
              WATCH_UI_EVENT_BLOCKING, false);
    set_event(&model->events[1],
              "pump.service-due", "PUMP-2", "СКОРО СЕРВИС",
              "ПРОВЕРКА ЧЕРЕЗ", "12 ЧАСОВ РАБОТЫ",
              WATCH_UI_EVENT_IMPORTANT, false);
    set_event(&model->events[2],
              "watch.sync", "ЧАСЫ", "СИНХР ВРЕМЕНИ",
              "ТЕСТОВЫЙ СНИМОК", "АКТУАЛЕН",
              WATCH_UI_EVENT_ORDINARY, true);
}

const char *watch_ui_quality_label(watch_ui_quality_t quality)
{
    switch (quality) {
        case WATCH_UI_QUALITY_GOOD: return "НОРМА";
        case WATCH_UI_QUALITY_UNCERTAIN: return "НЕТОЧНО";
        case WATCH_UI_QUALITY_BAD: return "ОШИБКА";
        case WATCH_UI_QUALITY_STALE: return "УСТАРЕЛО";
        case WATCH_UI_QUALITY_OFFLINE: return "НЕТ СВЯЗИ";
        default: return "НЕИЗВЕСТНО";
    }
}
