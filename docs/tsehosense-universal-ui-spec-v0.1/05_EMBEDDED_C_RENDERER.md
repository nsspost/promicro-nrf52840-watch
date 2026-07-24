# 5. Embedded renderer на C

## 5.1. Принципы

- C99/C11 без обязательного C++;
- fixed-size pools;
- отсутствие malloc в steady state;
- отсутствие рекурсивного layout;
- bounded parse time;
- integer/fixed-point geometry;
- backend-agnostic drawing API;
- частичная перерисовка;
- deterministic fallback;
- fuzzable binary parser.

## 5.2. Предлагаемые модули

```text
ts_ui_package       parser/validation/compatibility
ts_ui_model         normalized runtime view
ts_ui_layout        template selection and geometry
ts_ui_render        display-list generation
ts_ui_draw          backend interface
ts_ui_input         touch/buttons/crown mapping
ts_ui_events        presentation priority
ts_ui_commands      lifecycle client
ts_ui_skin          tokens/resources
ts_ui_strings       UTF-8, formatting, locale lookup
ts_ui_dirty         dirty-rectangle planning
```

## 5.3. Drawing API

Минимальные операции:

- clear/fill rect;
- horizontal/vertical line;
- glyph run;
- bitmap/icon blit;
- clipped polyline для sparkline;
- arc/gauge только если backend поддерживает;
- set clip/reset clip;
- flush dirty regions.

Иконки и сложная статика могут храниться bitmap-ресурсами. Анимации для
mono/low-power профиля заменяются дискретными фазами или статическим статусом.

## 5.4. Layout output

Layout engine создаёт плоский display list:

```c
typedef struct {
    uint16_t component_id;
    uint8_t  primitive;
    uint8_t  style_token;
    int16_t  x;
    int16_t  y;
    int16_t  w;
    int16_t  h;
    uint16_t binding_id;
    uint16_t flags;
} ts_ui_draw_item_t;
```

Точная структура определяется после измерения target. Важна плоская,
валидируемая и ограниченная форма.

## 5.5. Runtime state

Значения хранятся в tagged union с явной единицей и quality. Строковое
форматирование выполняется в фиксированный scratch buffer. Float не является
обязательным: входные значения могут храниться как integer + decimal scale.

## 5.6. Partial redraw

Binding знает зависимые draw items. При изменении значения:

1. сравнить revision/value/quality;
2. пометить component dirty;
3. объединить пересекающиеся прямоугольники;
4. если превышен `maxDirtyRects`, выполнить full redraw;
5. flush только изменившиеся области.

Переход между экранами на слабом процессоре:

- сначала контур/low-detail frame;
- затем полный статический screen;
- никаких обязательных 30/60 FPS.

## 5.7. Память

Для 256×256 mono1 ориентир:

- framebuffer: 8192 bytes;
- package metadata: 2–8 KB;
- runtime model: 2–6 KB;
- display list: 2–4 KB;
- scratch/strings: 1–2 KB;
- dirty regions: менее 256 bytes.

Это целевой бюджет, а не гарантия до реализации. IDE обязана считать реальные
размеры собранного package.

## 5.8. Input abstraction

Семантические события:

- `activate`;
- `back`;
- `next`;
- `previous`;
- `scroll`;
- `hold-start`;
- `hold-progress`;
- `hold-cancel`;
- `hold-complete`.

Touch, crown, buttons и keyboard преобразуются в эти события adapter-ом.

Guarded hold измеряется монотонным системным таймером. Потеря focus, pointer
cancel, переход экрана, важное/блокирующее событие или sleep отменяют hold.

## 5.9. Безопасность package parser

До активации:

- проверить magic/version/size;
- проверить CRC;
- проверить все offsets и counts;
- проверить string bounds и UTF-8 policy;
- проверить отсутствие циклов navigation;
- проверить actions против capability/risk policy;
- проверить budgets Display Profile;
- построить package handle только после полной валидации.

При ошибке включается встроенная readable fallback skin и read-only device
overview.

## 5.10. Conformance

Один набор golden scenarios прогоняется через:

- TypeScript reference compiler;
- web reference renderer;
- C package parser;
- C layout engine;
- C drawing recorder.

Сравнивать:

- выбранные templates;
- component order;
- bindings/actions;
- geometry с допустимым tolerance;
- fallback decisions;
- safety action availability.

