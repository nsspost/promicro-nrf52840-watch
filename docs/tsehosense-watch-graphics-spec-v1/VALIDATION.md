# Валидация графики

## Автоматические проверки

- [ ] Все JSON проходят parser и schema validation.
- [ ] Все layout bounds целочисленные и находятся в 256 × 256.
- [ ] Persistent controls не пересекают round clip.
- [ ] Hit targets не меньше 44 × 44, кроме недоступных декоративных glyph.
- [ ] Display list не превышает capacity.
- [ ] Dirty rect count ≤ 8.
- [ ] Peak graphics RAM ≤ 12288 bytes для mono1 profile.
- [ ] Все обязательные glyph и icon resources существуют.
- [ ] На missing glyph/asset есть явная ошибка и fallback.
- [ ] Full redraw и dirty redraw дают одинаковый итоговый framebuffer.

## Pixel QA

Для каждого эталона:

1. отрисовать PNG 256 × 256 без масштабирования;
2. собрать side-by-side и difference image;
3. сравнить canvas, clip, секции, baseline, divider и иконки;
4. проверить 1:1 и nearest-neighbor ×4;
5. записать объяснение любого допустимого различия.

Минимальный набор:

- home square/round normal;
- home square/round ambient;
- devices/overview/parameters square/round;
- ordinary/important/blocking/journal/detail square/round;
- control/threshold/band/sending/success/error/offline square/round;
- guarded/hold/critical/waiting/approved/denied square/round.

## Ручная проверка 256 px

- [ ] `20:42` не подрезано.
- [ ] `доступен` оптически связано с `AE-12`.
- [ ] Батарея и погода читаются без приближения.
- [ ] Телефон и экосистема имеют разные иконки/счётчики.
- [ ] `3 рядом` центрировано.
- [ ] На круге ни один persistent glyph не задет маской.
- [ ] Строки имеют ясный ритм, divider не толще текста.
- [ ] Значения заметнее подписей.
- [ ] Primary button явно инверсная.
- [ ] Selected enum понятен без цвета.
- [ ] Blocking event нельзя спутать с important.
- [ ] Approved critical request не предлагает выполнить команду.

## Оптика и доступность

- [ ] Проверено при физическом размере предполагаемого дисплея.
- [ ] Проверено с имитацией пониженного контраста.
- [ ] Важная информация не зависит от серого.
- [ ] Нет обязательного текста меньше 9 px на круге / 11 px на квадрате.
- [ ] Состояние имеет текст или glyph, а не только цвет.
- [ ] Touch, crown и физическая back дают один и тот же маршрут.

## Частичная перерисовка

- [ ] Минута меняет только time bounds.
- [ ] Батарея не очищает weather cell.
- [ ] Toast восстанавливает скрытую область после исчезновения.
- [ ] Blocking overlay замораживает нижнюю анимацию.
- [ ] После закрытия overlay нижний слой полностью корректен.
- [ ] Hold cancel не оставляет заполненные пиксели.

## Шкурка

- [ ] Signature/checksum и budget проверены до активации.
- [ ] Required semantic roles присутствуют.
- [ ] System overlay fallback не зависит от шкурки.
- [ ] Неуспешная активация сохраняет предыдущую шкурку.
- [ ] Preview использует тот же compiled IR и ресурсы, что firmware.

## Definition of done

Экран считается реализованным, когда есть:

- код/IR;
- state fixture;
- square/round, если применимо;
- PNG 1:1;
- comparison/diff;
- bounds tests;
- memory report;
- отсутствие P0/P1/P2 визуальных дефектов.
