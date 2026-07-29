# Инструкция Codex: графика часов ЦехоСенс

## Цель

Воспроизвести принятый дизайн «Строгий контекст» на host renderer и embedded C
без изменения его визуального языка и safety-семантики.

## Обязательное чтение

1. `README.md`
2. `01_VISUAL_LANGUAGE.md`
3. `02_TOKENS_AND_TYPOGRAPHY.md`
4. `03_LAYOUT_GEOMETRY_256.md`
5. `04_COMPONENT_LIBRARY.md`
6. `08_EMBEDDED_GRAPHICS_C.md`
7. `VALIDATION.md`

## Источник истины

Эталонные PNG и текущая спецификация важнее legacy manifest/layout. Не
перерисовывать дизайн «по мотивам». Сначала измерить и воспроизвести.

## Ограничения

- Canvas 256 × 256.
- Отдельные square и round profiles.
- Mono1 является обязательным.
- Никаких теней, градиентов, карточек и декоративных скруглений.
- Экранный шрифт — Roboto.
- Иконки — Phosphor из `icons/phosphor/`.
- Никаких emoji, Unicode-символов вместо иконок, самодельных SVG и CSS-art.
- Не кодировать состояние только цветом.
- Hit target не меньше 44 × 44.
- Layout coordinates целочисленные.
- Runtime не парсит SVG/TTF/JSON и не исполняет код шкурки.
- Системные blocking/critical overlays имеют встроенный fallback.

## Порядок работы

1. Исследовать текущий код и сообщить mapping файлов.
2. Добавить renderer-neutral IR.
3. Сделать host renderer и получить PNG.
4. Сравнить PNG с соответствующим reference при 1:1.
5. Исправить геометрию, font metrics, clips и иконки.
6. Только затем подключать embedded backend.
7. После каждого профиля запускать validation.

## Запреты

- Не менять протоколы BLE/LoRa.
- Не переносить React/CSS в firmware.
- Не использовать screenshot как runtime UI.
- Не масштабировать square layout и обрезать его кругом.
- Не заменять крупное значение ellipsis.
- Не подтверждать команду до ответа устройства.
- Не добавлять кнопку выполнения на экран `critical approved`.

## Отчёт о выполнении

Указать:

- изменённые файлы;
- использованный profile и target;
- RAM/flash;
- список реализованных экранов;
- результаты golden/pixel/bounds tests;
- известные visual diffs со ссылкой на comparison image;
- оставшиеся блокеры.
