# 07. Шкурки и ресурсы

## Разделение ответственности

Шкурка меняет визуальное представление, но не безопасность и семантику.

Шкурка может менять:

- font assets из разрешённого набора;
- размеры текстовых ролей в допустимом диапазоне;
- layout-профили;
- иконки из объявленного icon set;
- яркости gray4;
- толщину разрешённых линий;
- ambient-композицию.

Шкурка не может менять:

- уровень события;
- требование квитирования;
- класс команды `ordinary/guarded/critical`;
- hit action;
- confirmed/pending смысл;
- обязательное отображение двух потоков уведомлений;
- порядок слоёв безопасности.

## Предлагаемая структура

```text
skin/
  manifest.json
  tokens.json
  layouts/
    square-256.json
    round-256.json
  fonts/
    ui-regular.bin
    ui-medium.bin
    ui-bold.bin
  icons/
    regular-20.bin
    regular-24.bin
    regular-32.bin
    fill-20.bin
  strings/
    ru.json
  previews/
  license/
```

На устройство должен попадать не authoring JSON, а скомпилированный bounded
binary package без кода и произвольных выражений.

## Manifest

Обязательные поля:

- `format = tseho.graphics.skin`;
- `formatVersion`;
- `skinId`, `version`;
- список target profiles;
- `requiredSemanticRoles`;
- `requiredIcons`;
- memory budgets;
- checksum/signature;
- fallback skin.

## Ресурсные ограничения

- Все glyph/icon bounds известны до загрузки.
- Нет SVG/XML/JS parser на часах.
- IDE превращает TTF/WOFF и SVG в bitmap/vector-command resources.
- Каждый ресурс имеет width, height, baseline, advance, crc и byte length.
- Пакет отклоняется до активации, если превышен flash/RAM budget.
- Предыдущая рабочая шкурка сохраняется до успешной проверки новой.

## Размерные классы

Шкурка обязана поставлять отдельный layout хотя бы для каждого поддерживаемого
shape/size class:

- `square.compact.256`;
- `round.compact.256`;
- в будущем `square.medium`, `round.medium`, `band.large`.

Inheritance разрешён только в IDE. Compiler разворачивает его в полное
описание, чтобы embedded renderer не выполнял каскад.

## Сопоставление семантических ролей

Шкурка получает не сырые имена полей, а роли:

- `time.primary`;
- `watch.battery`;
- `weather.temperature`;
- `context.device.id`;
- `context.device.state`;
- `unread.phone`;
- `unread.ecosystem`;
- `nearby.count`;
- `event.severity`, `event.value`;
- `command.phase`, `command.action`.

Новая шкурка обязана иметь fallback для required roles.

## Предпросмотр в IDE

Preview использует тот же compiled layout IR, что и firmware:

- target shape/size/pixel profile;
- locale;
- demo data;
- event/command state;
- font rasterizer version;
- clipping and pixel rounding mode.

Нужны режимы:

1. 1:1 physical pixels;
2. увеличенный nearest-neighbor;
3. circular clip overlay;
4. dirty rect visualization;
5. mono1/gray4 simulation;
6. overflow warnings;
7. memory budget report.

## Совместимость с Chronos

Совместимость разумно строить как importer, а не как runtime-зависимость:

1. IDE читает поддерживаемые metadata/resource entries Chronos;
2. извлекает фон, bitmap, шрифтовые/позиционные данные, если формат известен;
3. сопоставляет доступные поля с семантическими ролями ЦехоСенс;
4. показывает неподдерживаемые элементы;
5. компилирует результат в собственный безопасный пакет.

Нельзя обещать пиксельную совместимость без зафиксированной версии формата.
RTOS и исходная GUI-библиотека Chronos не нужны, если importer работает с
данными. Интерактивные промышленные экраны и safety overlays остаются
системными компонентами ЦехоСенс и не импортируются из циферблата.

## Лицензии

Текущий прототип использует Roboto и Phosphor. Тексты лицензий находятся в
`licenses/`. При замене ресурсов IDE должна переносить их license metadata в
пакет сборки.
