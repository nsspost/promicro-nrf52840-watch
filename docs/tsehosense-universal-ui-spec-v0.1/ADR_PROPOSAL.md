# ADR proposal: Universal UI model and bounded renderers

Дата: 2026-07-23  
Статус: предлагается к принятию

## Решение

Интерфейсы устройств ЦехоСенс строить из семантических descriptor-ов,
нормализованной UI View Model, Display Profile и skin package. Датчики и
устройства не передают исполняемый UI-код или произвольную пиксельную
разметку. Для embedded-целей authoring model компилируется в bounded binary
package без general-purpose bytecode.

ЦехоЛаб становится основным authoring/validation/preview-инструментом. Шлюз
является основным runtime compiler/registry для часов и локальных дисплеев.
Web reference renderer и будущий C renderer используют одни template IDs,
fallback rules и safety invariants.

## Причина

Один и тот же датчик должен корректно отображаться на часах 256×256, панели
шлюза, браслете и web без отдельных прошивок интерфейса. Свободный метаязык
или device-supplied code усложнил бы безопасность, совместимость и embedded
реализацию. Только заранее ограниченные templates позволяют вычислять память,
гарантировать fallback и проверять опасные действия до загрузки package.

## Последствия

- passport/channel schemas получают необязательные presentation metadata;
- capabilities обязательно несут risk class и confirmed feedback contract;
- ЦехоЛаб получает Display Profiles, preview scenarios и UI diagnostics;
- создаётся Theia-независимый TypeScript `ui-model` package;
- создаётся binary `ts_ui_package_v1`;
- embedded C renderer использует fixed pools и плоский display list;
- skins меняют только визуальный слой;
- unknown risk всегда становится `critical`;
- `critical approved` никогда не создаёт execute action на часах;
- forcing/service-mode policy DEC-029 не ослабляется;
- catalog migrations DEC-036 распространяются на UI metadata.

## Отклонённые альтернативы

### Отдельные экраны в firmware для каждого устройства

Не масштабируется и связывает firmware часов с каталогом датчиков.

### Полная разметка экрана из устройства

Слишком тяжела для младших МК, плохо переносится между дисплеями и создаёт
недоверенную поверхность.

### HTML/CSS/JavaScript как универсальный runtime

Подходит web, но не соответствует памяти, энергопотреблению и bounded safety
embedded-целей.

### Только заранее нарисованные экраны без View Model

Просто на старте, но не решает разные наборы параметров, новые устройства и
панели другого размера.

