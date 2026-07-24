# 08. Реализация графики на C

## Архитектурная граница

Не переносить React/CSS на MCU. Переносится **результат дизайна**:

```text
semantic state
  → resolved screen model
    → layout profile
      → display list
        → raster backend
          → dirty rectangles
            → display driver
```

Renderer не знает BLE, LoRa и паспортный протокол. Он получает immutable
snapshot и генерирует кадр. Input router отдельно преобразует touch/button/crown
в semantic actions.

## Минимальные примитивы

```c
void gfx_fill_rect(gfx_rect_t r, gfx_color_t c);
void gfx_stroke_rect(gfx_rect_t r, uint8_t width, gfx_color_t c);
void gfx_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
              uint8_t width, gfx_color_t c);
void gfx_blit_mask(gfx_point_t p, const gfx_bitmap_t *mask, gfx_color_t c);
void gfx_draw_text(gfx_point_t p, const gfx_font_t *font,
                   gfx_string_view_t text, gfx_color_t c);
gfx_size_t gfx_measure_text(const gfx_font_t *font, gfx_string_view_t text);
void gfx_push_clip(gfx_rect_t r);
void gfx_push_round_clip(gfx_point_t center, uint16_t radius);
void gfx_pop_clip(void);
```

Не требуются canvas paths, gradients, shadows, arbitrary transforms и
runtime-SVG.

## Предлагаемые типы

```c
typedef enum { GFX_MONO_OFF = 0, GFX_MONO_ON = 1 } gfx_color_t;

typedef struct { int16_t x, y, w, h; } gfx_rect_t;
typedef struct { int16_t x, y; } gfx_point_t;
typedef struct { uint16_t w, h; } gfx_size_t;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t stride_bytes;
    const uint8_t *data;
} gfx_bitmap_t;

typedef struct {
    uint16_t glyph_count;
    int16_t ascent;
    int16_t descent;
    int16_t line_gap;
    const void *glyphs;
    const uint8_t *bitmap;
} gfx_font_t;
```

Точные имена не обязательны; важны bounded data и отсутствие allocation при
отрисовке.

## Framebuffer

Для 256 × 256 × 1 bpp полный framebuffer занимает 8192 bytes. Базовый budget:

- framebuffer 8192;
- display list/dirty rect/state scratch — до 4096;
- peak RAM для графического прохода — 12288 bytes.

Если драйвер допускает strip rendering, API всё равно должен сохранить
детерминированный clip и порядок слоёв.

## Display list

Рекомендуется сначала строить компактный список:

```c
typedef enum {
    GFX_CMD_FILL_RECT,
    GFX_CMD_LINE,
    GFX_CMD_TEXT,
    GFX_CMD_BITMAP,
    GFX_CMD_PUSH_CLIP,
    GFX_CMD_POP_CLIP
} gfx_cmd_kind_t;
```

Плюсы: один layout работает для mono framebuffer, desktop preview и golden
tests; легко считать bounds и dirty rect; backend можно заменить.

Display list bounded: фиксированная ёмкость, явная ошибка overflow, никакого
тихого пропуска.

## Текст

- Шрифт компилируется в bitmap atlas для реально используемых glyph.
- Обязательная кириллица определяется словарём UI и паспортными ограничениями.
- Числовой font subset включает `0123456789.,:−+%°/`.
- Runtime UTF-8 decoder обязан заменять отсутствующий glyph явным `□` и
  устанавливать diagnostic flag.
- Для ключевого значения overflow является layout error, не ellipsis.
- Для subtitle/eyebrow разрешён ellipsis с предварительно растеризованным `…`.
- Tabular цифры обязательны для времени и быстро меняющихся значений.

## Иконки

Исходные SVG Phosphor компилируются в 1-bit masks на фиксированных размерах.
Нельзя масштабировать маленький bitmap в runtime. Минимальные buckets:

- 18, 20, 24, 28, 32;
- 45/46 для статуса;
- 55/64 контейнеры не означают glyph такого же размера.

После растрирования проверять, что regular stroke не исчез и замкнутые области
не слиплись. Fill-вариант хранится отдельным mask.

## Pixel snapping

- Bounds и baseline целочисленные.
- Горизонтальные/вертикальные линии рисуются на целых координатах.
- Stroke всегда направлен внутрь declared bounds.
- Не полагаться на subpixel anti-aliasing.
- Крупный Roboto может потребовать ручной optical y-offset, зафиксированный в
  layout, особенно время и `доступен`.

## Round clip

Для каждого scanline можно хранить заранее вычисленные `x_min[y]` и `x_max[y]`.
Это быстрее, чем проверка окружности на каждый пиксель. Компонент сначала
clipped собственным bounds, затем shape clip.

## Состояние и перерисовка

```c
typedef struct {
    watch_screen_id_t screen;
    watch_shape_t shape;
    watch_mode_t mode;
    watch_view_data_t data;
    watch_overlay_t overlay;
    uint32_t revision;
} watch_render_snapshot_t;
```

Snapshot не меняется во время render pass. После сравнения с предыдущим
snapshot component mapper выдаёт dirty regions. Если изменился layout/shape,
делается full redraw.

## Input и hit map

Hit targets поставляются layout profile и могут быть больше визуальных bounds.
Renderer не определяет действие по координатам нарисованной иконки.

Приоритет:

1. blocking overlay;
2. important overlay;
3. critical request;
4. command status;
5. toast;
6. current screen.

## Инверсия

Для белой выбранной строки или кнопки:

1. fill белым;
2. текст/иконку рисовать чёрным;
3. не использовать XOR как единственную реализацию — результат должен быть
   одинаков на полном и частичном redraw.

## Ошибки

При невозможности отрисовать шкурку:

- записать diagnostic code;
- откатиться к `builtin.readable`;
- не пытаться продолжать с частично загруженным font/icon resource;
- blocking/critical системные экраны обязаны существовать во встроенном
  fallback независимо от пользовательской шкурки.

## Golden tests

Desktop/backend renderer принимает тот же compiled IR и state fixture, выдаёт
256 × 256 PNG. Для каждого эталона:

- exact canvas/clip;
- pixel-diff;
- отдельная tolerance mask только для заранее объяснённого отличия rasterizer;
- OCR/metrics check для ключевых строк;
- bounds assertion для каждого persistent control.

Первый acceptance threshold после согласования font rasterizer: не менее 98,5%
совпадающих пикселей вне текста и отдельный визуальный review текста. После
фиксации rasterizer — exact golden для той же платформы.
