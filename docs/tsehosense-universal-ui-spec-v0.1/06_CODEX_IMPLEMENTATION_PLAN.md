# 6. План реализации для Codex

## 6.1. Обязательный порядок работы

Codex не должен сразу писать embedded renderer. Сначала требуется стабилизировать
контракт и reference implementation.

## Фаза 0 — разведка репозитория

1. Прочитать корневые `AGENTS.md`, `PROJECT_INDEX.md`, `DECISIONS.md`.
2. Найти реальное дерево ЦехоЛаб, catalog, channel schemas, hardware service,
   project model, validators и tests.
3. Найти существующие C common libraries и правила их сборки.
4. Найти форматы passport/schema/variable map/service protocol.
5. Составить mapping предложенных путей к существующим.
6. Не создавать параллельную архитектуру, если аналог уже есть.

Результат: `docs/ui/UI_REPOSITORY_MAPPING.md`.

## Фаза 1 — ADR и схемы

Добавить ADR:

`Universal UI uses semantic descriptors, normalized View Model and bounded
renderers; device-supplied executable UI is forbidden.`

Добавить JSON Schema:

- display profile;
- presentation metadata;
- capability UI extension;
- UI View Model;
- skin manifest;
- preview scenario.

Расширить catalog validator. Никакого UI пока не строить.

Acceptance:

- существующий catalog проходит;
- PUMP-2 example проходит;
- несовпадение unit/role/risk обнаруживается;
- unknown optional hint игнорируется;
- unknown required semantic отклоняется.

## Фаза 2 — ui-model package

Создать чистый TypeScript package без Theia dependency:

- loaders;
- normalization;
- sorting;
- visibility;
- template selection;
- fallbacks;
- diagnostics;
- deterministic serialization.

Golden tests:

- AE-12 overview;
- SIG-4 controls;
- PUMP-2 guarded/critical;
- long labels;
- offline/stale;
- missing skin kind;
- square/round/panel profiles.

Acceptance: одинаковый input даёт byte-identical normalized JSON.

## Фаза 3 — reference renderer

Перенести существующий дизайн «Строгий контекст» на вход из View Model:

- убрать hardcoded AE-12/SIG-4/PUMP-2 из render components;
- mock data оставить только preview scenarios;
- все actions создавать из capability contracts;
- системный event/command precedence вынести в surface manager.

Не менять визуальный язык без отдельного design decision.

Acceptance:

- существующие 256×256 evidence states воспроизводятся;
- square и round не имеют clipped controls;
- critical approval не создаёт execute action;
- blocking event preempts command/request.

## Фаза 4 — Theia integration

Добавить:

- preview widget;
- display/skin selectors;
- parameter/capability inspector;
- Problems diagnostics;
- scenario selector;
- compile package command.

Использовать существующие Theia service/widget patterns. UI-model package не
зависит от Theia.

Acceptance:

- изменение priority сразу перестраивает preview;
- смена display profile не меняет passport;
- ошибки появляются в Problems;
- проект сохраняется без React-specific state.

## Фаза 5 — binary compiler

Определить `ts_ui_package_v1`:

- reproducible output;
- explicit endianness;
- CRC32;
- bounded tables;
- package report;
- optional signature placeholder.

Добавить inspector/dump tool и malformed corpus.

Acceptance:

- compile → dump сохраняет semantics;
- одинаковый input даёт одинаковый binary;
- corrupted offsets/CRC отклоняются;
- budget overflow останавливает build.

## Фаза 6 — C parser and model

Реализовать:

- parser;
- validator;
- fixed pools;
- string lookup;
- binding update;
- action validation.

Сначала host tests, затем target.

Acceptance:

- ASan/UBSan host tests;
- malformed corpus без crash/out-of-bounds;
- PUMP-2 package загружается;
- critical action остаётся request-only.

## Фаза 7 — C layout/drawing recorder

Реализовать templates без аппаратного дисплея. Backend пишет display list в
golden text/JSON.

Acceptance:

- geometry fits safe area;
- component count bounded;
- dirty dependency list корректен;
- web/C template decisions совпадают.

## Фаза 8 — первый дисплей

Выбрать один target, вероятно часы 256×256 mono1 или дисплей шлюза, в
зависимости от готовности железа.

Реализовать:

- framebuffer backend;
- fonts/icons;
- input adapter;
- package storage;
- fallback skin;
- performance counters.

Acceptance:

- cold load;
- navigation;
- partial redraw;
- event preemption;
- guarded hold cancellation;
- power-loss/package corruption fallback.

## Фаза 9 — gateway distribution

Добавить registry/View Model/resources endpoints поверх существующего
transport abstraction. BLE/LoRa — adapters, не новая модель.

Acceptance:

- descriptor revision cache;
- reconnect;
- expired commands не выполняются;
- duplicate idempotency key не повторяет действие;
- incompatible package не активируется.

## 6.2. Запреты для Codex

- не переносить React в embedded;
- не исполнять layout/script от устройства;
- не хранить safety policy в skin;
- не делать optimistic confirmed state;
- не добавлять deferred commands по умолчанию;
- не строить второй catalog рядом с существующим;
- не связывать View Model напрямую с MQTT topic names;
- не добавлять malloc без измеренного обоснования;
- не ослаблять current forcing/service-mode policy;
- не считать screenshot достаточной проверкой без comparison/interaction tests.

## 6.3. Definition of Done первой большой итерации

- ADR принят;
- схемы и validators работают;
- View Model compiler покрыт golden tests;
- ЦехоЛаб показывает 5 Display Profiles;
- PUMP-2 проходит полный preview scenario;
- binary package v1 воспроизводим;
- C parser проходит malformed tests;
- один C layout recorder совпадает с reference semantics;
- документация содержит реальные пути и команды сборки;
- `DECISIONS.md`, `PROJECT_INDEX.md` и backlog обновлены.

