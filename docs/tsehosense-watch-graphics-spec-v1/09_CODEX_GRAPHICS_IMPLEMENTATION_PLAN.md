# 09. План внедрения для Codex

План предполагает, что архитектурный протокол и IDE развиваются отдельно.
Графический слой не должен ждать готовности часов: сначала нужен host renderer.

## Этап 0. Разведка репозитория

Codex должен:

1. найти существующие display/gui/font/asset модули;
2. найти hardware abstraction и ограничения display controller;
3. найти модель паспорта и UI IR из архитектурного пакета;
4. найти систему сборки IDE и firmware;
5. не менять транспорт и safety logic ради удобства renderer.

Результат: короткий mapping «существующий файл → новая ответственность».

## Этап 1. Renderer-neutral IR

Добавить:

- shape/size/pixel profiles;
- semantic color/font/spacing tokens;
- component bounds и hit targets;
- screen/display-list commands;
- immutable render snapshot;
- validation bounds/budget.

IR не должен зависеть от LVGL, React, конкретного дисплея или JSON runtime.

## Этап 2. Host reference renderer

Сделать desktop/CLI renderer, который:

- читает compiled fixture;
- рисует mono1 256 × 256;
- сохраняет PNG;
- умеет square/round;
- использует те же bitmap fonts/icons, что firmware;
- запускает golden tests по `reference/screens/`.

Это главный инструмент совпадения графики до переноса на MCU.

## Этап 3. Asset compiler в IDE

Compiler:

- валидирует `schemas/graphics-skin.schema.json`;
- переводит tokens/layouts в полные profiles;
- растеризует Roboto и Phosphor;
- строит glyph/icon atlases;
- считает RAM/flash;
- выдаёт binary package + map report + preview fixture;
- запрещает runtime code/expression.

## Этап 4. Главный экран

Реализовать square normal, затем round normal, затем ambient.

Definition of done:

- точная геометрия;
- два независимых уведомления;
- device row;
- hit map 44 px;
- dirty regions;
- эталонные PNG.

## Этап 5. Базовые компоненты

Порядок:

1. ScreenHeader;
2. ListRow;
3. PrimaryMetricPair;
4. ActionSplit;
5. NumericEditor и EnumSelector;
6. EventToast и overlays;
7. CommandOverlay;
8. HoldConfirm.

Каждый компонент сначала проходит host golden test, затем переносится/подключается
к target backend.

## Этап 6. Embedded backend

Добавить mono framebuffer, clips, bitmap text/icons, display list execution,
dirty rect flush и input hit map. Проверить memory budget на реальном target.

Не добавлять полноценную GUI-библиотеку, если эти примитивы уже покрывают
контракт. Если используется библиотека, спрятать её за renderer interface.

## Этап 7. IDE preview

Показывать:

- shape;
- 1:1 и zoom-nearest;
- mono1/gray4;
- состояние данных;
- overlays;
- circular safe mask;
- bounds/hit areas/dirty rect;
- overflow и resource budget.

Preview и firmware обязаны использовать один compiled IR.

## Этап 8. Шкурки

Добавить install/validate/activate/rollback. System safety overlay и fallback
font/icons всегда встроены в firmware. Пользовательская шкурка не может
отключить required roles.

## Стратегия коммитов

Предпочтительные независимые изменения:

1. `graphics-ir`;
2. `host-renderer`;
3. `asset-compiler`;
4. `strict-context-home`;
5. `components`;
6. `events-and-commands`;
7. `embedded-backend`;
8. `ide-preview`;
9. `skin-runtime`.

Не смешивать визуальную настройку, transport protocol и safety semantics в
одном изменении.

## Вопросы, которые Codex должен задать до target backend

- точная модель дисплея и способ адресации framebuffer;
- mono1 или gray4;
- доступная RAM/flash;
- наличие FPU/ускорителя;
- touch/crown/buttons;
- допустимая частота partial refresh;
- способ хранения внешней шкурки;
- лицензирование/способ включения Roboto.

До ответов можно полностью завершить IR, host renderer, assets и golden tests.
