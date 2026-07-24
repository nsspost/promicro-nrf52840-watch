# Текущее состояние и план экспериментальных часов

Дата ревизии: 2026-07-24
Статус: аппаратная цель подтверждена; полноценный GUI на тестовых данных
собран, прошит и проверен

## 1. Что уже есть

### Аппаратная основа

- ProMicro-плата с подтверждённым `nRF52840_XXAA`.
- SWD через J-LinkOB работает стабильно на 1 МГц.
- Целевое питание при отладке: 3,300 В.
- Cortex-M4F определяется, Flash программируется и проверяется.
- До первой записи сохранены полный Flash 1 МиБ и UICR.
- Диагностическая прошивка запускается; RAM heartbeat проверен через J-Link.
- Подтверждены круглый дисплей GC9A01 240×240 RGB565 и touch CST816D.

### Firmware и инструменты

- минимальный C11 bare-metal startup;
- linker script для 1 МиБ Flash и 256 КиБ RAM;
- локально закреплённые ARM GCC, CMake и Ninja;
- SEGGER probe/flash/GDB scripts;
- автоматический backup перед первой записью;
- VS Code tasks и Cortex-Debug configuration;
- воспроизводимые `build`, `probe`, `flash`, `verify-ui`, `verify-running`;
- optional CMake integration с NOG_C.

Текущий firmware содержит драйверы дисплея и touch, framebuffer-free NOG_C
backend и 11-экранный GUI на тестовой View Model. Он пока не содержит BLE,
scheduler/RTOS, USB application, энергосбережение, файловое хранилище и
подключение реальных TsehoSense-данных.

### NOG_C

Подключён отдельным Git-репозиторием:

```text
external/NOG_C
origin: https://github.com/nsspost/NOG_C.git
revision: 963e61516253ddd75e4edc5e0b80d4528fb4f2d1
```

На текущей ревизии есть:

- `MONO1`, `RGB565`, `XRGB8888`;
- memory и framebuffer-free streaming backends;
- pixel/line/rectangle/bitmap/text primitives;
- clipping, viewport stack и dirty rectangle;
- окна и статические элементы;
- current/next bounds и damage restoration;
- host tests и SDL3 demo.

Это раннее графическое ядро, а не semantic UI framework. Для реального экрана
часов ему ещё потребуются измеренный display backend, проверка стоимости
операций, возможно несколько dirty rects, дополнительные glyph/asset решения
и гарантии для выбранного pixel format. Эти изменения должны оставаться общими
и проверяться внутри NOG_C.

### TsehoSense Universal UI

Контракты v0.1 находятся непосредственно в этом репозитории:

```text
docs/tsehosense-universal-ui-spec-v0.1
revision: v0.1
```

Там находятся спецификация, схемы, Display Profiles, PUMP-2 examples, ADR
proposal и правила реализации. Watch-first renderer использует их как
контракт, но binary compiler/package parser пока не реализован.

Главная граница:

```text
Universal UI: semantics, model, layout, safety, package
NOG_C:        generic drawing and damage handling
Watch:        hardware, transports, storage, power, integration
```

## 2. Чего пока нет

### Неизвестно по аппаратуре

- точное название и схема ProMicro-клона;
- соответствие выводов маркировке платы;
- наличие/тип внешнего 32,768 кГц кварца;
- тип дисплея и контроллера;
- разрешение, форма, pixel format и интерфейс дисплея;
- touch/buttons/crown;
- аккумулятор, charger, fuel gauge и power-path;
- вибромотор, buzzer, подсветка и enable-линии;
- датчики на плате;
- доступность NFC/USB/QSPI и их разводка.

Без этого нельзя корректно выбрать Display Profile, framebuffer strategy,
частоту обновления и power budget.

### Не выбрана программная платформа

Текущий bare-metal достаточен для bring-up, но не решено, что использовать для
продуктового прототипа:

- nRF Connect SDK/Zephyr;
- Nordic nrfx + SoftDevice Controller;
- другой BLE stack;
- собственный небольшой scheduler.

Для телефона, безопасных обновлений, bonding, GATT, low-power timers и
диагностики наиболее вероятен Zephyr/NCS, но решение принимается только после
проверки памяти, bootloader и требований к лицензированию/обновлению.

### Не определены контракты связей

- phone companion role и GATT services;
- pairing, ownership и recovery;
- descriptor/snapshot/event/command framing;
- fragmentation и MTU;
- clock/time sync;
- transport security и доверие к источнику;
- offline cache и retention;
- протокол обнаружителя;
- способ подключения часов к шлюзу/серверу ТехноСенс;
- OTA firmware и UI package distribution.

BLE, LoRa, USB и MQTT должны быть adapters над общей моделью, а не источниками
разных UI semantics.

### Не реализован Universal UI

Отсутствуют:

- repository mapping к реальному TsehoSense/ЦехоЛаб;
- принятый ADR;
- расширенные validators;
- независимый TypeScript reference model;
- deterministic View Model compiler;
- web/reference renderer;
- golden scenario runner;
- binary package v1;
- host C parser/model/layout;
- Universal UI → NOG_C adapter;
- fallback UI/skin.

## 3. Предлагаемая системная архитектура

```text
                         ┌──────────────────────┐
                         │ Phone companion      │
                         │ time/config/network  │
                         └──────────┬───────────┘
                                    │ BLE adapter
┌──────────────────────┐            │
│ Detector             ├────────────┤ protocol adapter
└──────────────────────┘            │
                                    ▼
                         ┌──────────────────────┐
┌──────────────────────┐ │ Watch runtime        │
│ TsehoSense gateway   ├─┤ registry/cache      │
│ descriptors/events   │ │ event/command client│
└──────────────────────┘ └──────────┬───────────┘
                                    │ normalized state
                                    ▼
                         ┌──────────────────────┐
                         │ Universal UI         │
                         │ package/model/layout │
                         └──────────┬───────────┘
                                    │ draw operations
                                    ▼
                         ┌──────────────────────┐
                         │ NOG_C                │
                         └──────────┬───────────┘
                                    │ spans/dirty regions
                                    ▼
                         ┌──────────────────────┐
                         │ Display driver       │
                         └──────────────────────┘
```

Телефон может быть транспортным шлюзом, источником времени/настроек и
companion UI, но не должен становиться обязательным владельцем semantics
ТехноСенс. Обнаружитель — отдельный источник с adapter-ом. Часы хранят source
identity и явно показывают freshness/quality каждого значения.

## 4. План до работающих часов

План разделён воротами. Следующая фаза не начинается, пока не получены
артефакты и измерения предыдущей.

### Фаза 0 — зафиксировать железо

Работа:

1. Идентифицировать точную плату и получить схему/pinout.
2. Составить board manifest: GPIO, clocks, flash/RAM, USB, NFC, QSPI.
3. Выбрать или идентифицировать дисплей, ввод и питание.
4. Проверить линии мультиметром/логическим анализатором до drive GPIO.
5. Измерить idle/current при reset и минимальном firmware.

Результат:

- `boards/<board>/board.md`;
- pin map с источником и статусом проверки;
- схема дисплея/ввода/питания;
- выбранный реальный Display Profile;
- начальный power budget.

Стоп-условие: неизвестные GPIO питания или дисплея.

### Фаза 1 — выбрать platform baseline

Работа:

1. Сделать короткий NCS/Zephyr spike: boot, timer, BLE advertising, sleep,
   J-Link debug, size/RAM.
2. Сравнить с текущим bare-metal baseline.
3. Выбрать boot/update strategy и memory map.
4. Зафиксировать ADR программной платформы.

Результат:

- воспроизводимая сборка;
- BLE advertising;
- переход sleep/wake;
- таблица Flash/RAM/current;
- решение по bootloader/OTA.

### Фаза 2 — дисплей и ввод без Universal UI

Работа:

1. Написать минимальный display transport driver.
2. Реализовать NOG_C backend без изменений semantic слоя.
3. Вывести primitives/font/bitmap test pattern.
4. Добавить input adapter и монотонный timer.
5. Измерить полный и частичный redraw, RAM, SPI/QSPI bandwidth и ток.

Результат:

- аппаратный NOG_C example;
- evidence изображений и performance counters;
- список общих улучшений NOG_C отдельными upstream-задачами;
- подтверждённые budgets Display Profile.

### Фаза 3 — стабилизировать Universal UI независимо

Работа ведётся в `tsehosense-universal-ui`, не в firmware:

1. Сопоставить спецификацию с реальным TsehoSense и ЦехоЛаб.
2. Принять/скорректировать ADR.
3. Стабилизировать JSON Schema и validators.
4. Реализовать Theia-независимый reference model/compiler.
5. Реализовать reference renderer и golden scenarios.
6. Подтвердить square/round/panel profiles и safety precedence.

Результат:

- byte-identical normalized JSON для одинакового входа;
- PUMP-2 и простой detector scenario;
- reference preview;
- контракт, который можно переносить в binary/C.

Стоп-условие: reference model ещё меняет semantics или template decisions.

### Фаза 4 — compiled package и host C runtime

Работа:

1. Отдельным ADR определить `ts_ui_package_v1`.
2. Создать reproducible compiler, dump/inspect tool и malformed corpus.
3. Реализовать C parser/validator с fixed pools.
4. Реализовать C model/layout и drawing recorder.
5. Сравнить решения C и reference renderer golden-тестами.
6. Реализовать backend-neutral display list и отдельный NOG_C adapter.

Результат:

- deterministic binary package;
- ASan/UBSan host tests;
- bounded memory report;
- отсутствие NOG_C/TsehoSense/phone knowledge в core слоях;
- fallback на повреждённом/несовместимом package.

### Фаза 5 — первый UI на часах

Работа:

1. Загрузить встроенный package и skin.
2. Реализовать `home-context`, navigation и device overview.
3. Подключить input semantics.
4. Реализовать dirty updates и sleep/wake restore.
5. Проверить long label, stale/offline, low-memory и corrupt package.

Результат:

- локально работающие часы с тестовыми snapshots;
- измеренные Flash/RAM/stack/frame time/current;
- совпадение с reference golden scenario.

### Фаза 6 — телефон как первый реальный транспорт

Работа:

1. Зафиксировать минимальный GATT/protocol contract.
2. Реализовать pairing/bonding и ownership recovery.
3. Передавать time, descriptor, snapshot, event и command lifecycle.
4. Сделать минимальный companion harness/app для тестов.
5. Проверить reconnect, duplicates, TTL, MTU fragmentation и offline.

Результат:

- телефон обновляет контекст часов;
- события и подтверждённые значения переживают reconnect;
- desired не отображается confirmed до результата.

### Фаза 7 — обнаружитель

Работа:

1. Описать identity, параметры, quality, события и capabilities обнаружителя.
2. Реализовать protocol adapter без device-specific UI screen.
3. Создать detector golden scenario.
4. Проверить обычные/важные/блокирующие события и журнал.

Результат: обнаружитель появляется как новый source в уже существующих
template families.

### Фаза 8 — интеграция с ТехноСенс

Работа:

1. Использовать существующие registry/passport/channel schemas.
2. Подключить gateway View Model/resource distribution.
3. Реализовать command и critical authority lifecycle.
4. Интегрировать preview/validation с ЦехоЛаб.
5. Проверить migration, package compatibility и revoked resources.

Результат: часы становятся renderer-ом экосистемы, не создавая параллельный
catalog.

### Фаза 9 — надёжность носимого устройства

- power profiling по состояниям;
- watchdog, crash record и recovery;
- OTA rollback;
- storage wear/corruption;
- BLE security review;
- accessibility и outdoor readability;
- thermal/battery limits;
- длительные reconnect/sleep tests;
- корпус, антенна и реальные условия ношения.

## 5. Ближайшее решение

Следующий разумный шаг — не писать UI и не добавлять BLE. Сначала нужны фото
обеих сторон платы, маркировка/ссылка на конкретную ProMicro-nRF52840, схема
подключаемого дисплея и перечень имеющихся органов ввода/питания.

После этого можно выполнить Фазу 0 и решить, подходит ли существующий
square/round 256×256 mono1 profile или нужен новый профиль.

## 6. Решения, требующие отдельного ADR

1. Firmware platform: NCS/Zephyr или другой baseline.
2. Bootloader, DFU/OTA и memory map.
3. Binary UI package v1.
4. Package/skin trust and signature policy.
5. Phone companion ownership/security.
6. Critical authority model для реального управления ТехноСенс.
7. Offline retention и журнал.
8. Версионирование transport framing.

