# 3. Изменения по компонентам системы

## 3.1. Датчиковый контроллер

### Добавить

- стабильные parameter/channel IDs;
- type, unit, scale, bounds и quality;
- threshold metadata;
- role и visibility по умолчанию;
- priority и compact label ID;
- schema revision;
- capability IDs для реально поддерживаемых app-команд;
- подтверждённый revision состояния.

### Не добавлять

- layout coordinates;
- шрифты и иконки;
- skin resources;
- JSON parser на CH32V003;
- web-разметку;
- произвольные UI-команды.

На младшем МК хранится компактный бинарный descriptor. Строки могут быть
заменены стабильными ID каталога; полные локализованные подписи добавляет
шлюз/IDE.

### Protected supervisor

Supervisor владеет:

- board identity;
- совместимостью schema/app;
- паспортным CRC;
- channel protocol;
- проверкой команды;
- confirmed revision.

Sensor app владеет:

- получением значения;
- вычислением quality/fault;
- прикладными app-командами;
- рекомендуемыми thresholds и presentation hints.

## 3.2. Блок сбора уровня 1.1

Добавить `ts_ui_descriptor_cache`:

- чтение descriptor-ов подключённых модулей;
- проверка schemaVersion/CRC;
- объединение с installation passport;
- кэширование compact descriptor;
- генерация source snapshot;
- передача descriptor revision вверх;
- отказ от повторной передачи неизменившегося descriptor-а.

Блок 1.1 не обязан строить полный экран. Он должен уметь отдать достаточно
данных, чтобы шлюз восстановил семантическую модель без знания конкретного
target firmware.

## 3.3. Блок управления уровня 1.2 / PLC runtime

Добавить capability registry, связанный с process image:

- read binding;
- desired/write binding;
- confirmed/status binding;
- safety precondition;
- risk class;
- allowed value domain;
- requiresOnline;
- allowDeferred;
- estimated duration.

`%Q desired` не считается confirmed state. UI показывает новое состояние
только после отдельного status/feedback канала или подтверждённого результата.

Forcing и service mode остаются отдельным инженерным контуром. Обычный UI
capability не получает forcing автоматически.

## 3.4. Концентратор

Требования минимальны:

- сохранять source identity;
- не терять revision и timestamps;
- буферизовать события и lifecycle команды;
- не переопределять risk class;
- передавать descriptor hash/version.

## 3.5. Шлюз

Шлюз является основным UI model compiler для локальных renderer-ов.

Добавить сервисы:

### Device Registry

Хранит:

- discovered devices;
- board identity;
- installation passport;
- channel schemas;
- capabilities;
- current snapshot;
- online/freshness.

### View Model Compiler

Принимает registry + Display Profile + user policy и выдаёт normalized View
Model. Результат кэшируется по:

```text
device revision
catalog version
display profile
skin compatibility
policy revision
```

### Event Broker

Реализует severity, deduplication, unread, acknowledgement и resolution.
Состояния не должны существовать только в конкретных часах.

### Command Broker

Реализует:

- command ID;
- TTL;
- idempotency;
- precondition revision;
- lifecycle;
- manual retry;
- запрет deferred execution по умолчанию.

### Critical Authority Broker

Отделён от Command Broker. Создаёт запрос, но не трактует `approved` как
исполнение. Контур полномочий определяет, где появится реальное действие:
панель оператора, service workstation, физический ключ или PLC policy.

### Resource Distribution

Отдаёт:

- display profiles;
- skin manifests/packages;
- locale tables;
- icon/resource packs;
- совместимые compiled UI packages.

## 3.6. MQTT/server contract

Рекомендуемые логические потоки независимо от точного транспорта:

- descriptor/passport — retained, по revision;
- snapshot — compact current state;
- events — append/lifecycle;
- commands — request/result;
- critical requests — отдельный namespace;
- resources — versioned manifest, не telemetry.

UI model можно передавать локально часам по BLE/LoRa в бинарном виде, а web
строить на сервере из тех же исходных semantics. Серверный JSON и embedded
binary обязаны иметь общий conformance test.

## 3.7. Часы

Добавить независимые модули:

- transport adapters: BLE, LoRa, phone companion;
- registry cache;
- UI package loader;
- renderer core;
- input mapper;
- event surface manager;
- command lifecycle client;
- skin manager;
- journal/cache with bounded retention.

Часы не должны знать device-specific C screens для AE-12, SIG-4 или PUMP-2.
Исключения допустимы только как built-in fallback templates, не как логика
конкретного устройства.

## 3.8. Локальный дисплей шлюза

Использует тот же renderer core и UI package, но другой Display Profile.
Дополнительно может:

- показывать больше параметров;
- использовать постоянный журнал;
- иметь аппаратные softkeys;
- показывать commissioning diagnostics;
- быть внешним authority UI только при отдельном safety решении.

Наличие большого дисплея само по себе не делает critical-команду разрешённой.

## 3.9. Сервер/web

Reference renderer web-уровня:

- использует тот же View Model;
- может расширять шаблон графиками и историей;
- не изменяет semantics;
- должен иметь режим эмуляции ограниченного дисплея;
- является визуальным oracle для IDE и embedded conformance.

## 3.10. ЦехоЛаб

Добавить продуктовые сущности первого класса:

- Display Profiles;
- Skins;
- Presentation metadata;
- Capabilities and risk;
- View Model preview;
- UI package compiler;
- renderer diagnostics;
- compatibility matrix.

Предлагаемые каталоги:

```text
tools/plc/tsehosense-ide/catalog/display-profiles/
tools/plc/tsehosense-ide/catalog/skins/
tools/plc/tsehosense-ide/packages/ui-model/
tools/plc/tsehosense-ide/packages/ui-compiler/
tools/plc/tsehosense-ide/packages/ui-preview/
schemas/ui/
firmware/common/tsehosense_ui/
```

Codex обязан сначала сопоставить их с реальным деревом репозитория и
использовать существующие package/service patterns ЦехоЛаб.

