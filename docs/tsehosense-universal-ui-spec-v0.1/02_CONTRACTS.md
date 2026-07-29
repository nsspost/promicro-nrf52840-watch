# 2. Контракты данных

## 2.1. Дополнение к паспорту параметра

Базовый паспорт уже существует. В него следует добавить необязательный блок
`presentation`, не смешивая его с измерительной семантикой:

```json
{
  "parameterId": "pressure.outlet",
  "channelSchema": "measurement.pressure.v1",
  "label": "Давление",
  "compactLabel": "ДАВЛЕНИЕ",
  "unit": "bar",
  "presentation": {
    "visibility": "primary",
    "priority": 90,
    "preferredKinds": ["number", "gauge", "sparkline"],
    "sizeHint": "large",
    "group": "process",
    "precision": 1,
    "showTrend": true
  }
}
```

### Обязательная семантика остаётся вне presentation

- type;
- unit;
- scale/offset;
- quality;
- thresholds;
- read/write direction;
- role;
- command risk.

`presentation` разрешено игнорировать полностью. Без него renderer обязан
получить корректный, пусть и простой интерфейс.

## 2.2. Visibility

Поддержать:

- `primary` — показывать в сводке;
- `secondary` — показывать при наличии места;
- `details` — только таблица/детали;
- `hidden` — не показывать пользователю, но оставить для диагностики/логики.

`hidden` не используется для сокрытия активного fault, blocking event или
safety status.

## 2.3. Priority

Диапазон 0–100:

- 90–100 — жизненно важный главный параметр;
- 70–89 — основной рабочий;
- 40–69 — полезный вторичный;
- 1–39 — диагностический;
- 0 — только по явному запросу.

При равном priority сортировать по `passportOrder`, затем по стабильному ID.

## 2.4. Capability

Capability описывает действие:

```json
{
  "capabilityId": "drive.mode",
  "label": "Режим",
  "compactLabel": "РЕЖИМ",
  "controlType": "enum",
  "choices": ["AUTO", "MANUAL"],
  "riskClass": "guarded",
  "confirmation": "hold-1500-ms",
  "requiresOnline": true,
  "allowDeferred": false,
  "preconditionBinding": "state.revision"
}
```

Класс риска хранится в доверенном device/catalog policy. Паспорт установки
может только повысить риск для конкретного объекта. IDE и renderer также могут
повысить риск локальной политикой, но ни один слой не может его понизить.

## 2.5. Device snapshot

Snapshot является неизменяемым снимком подтверждённого состояния:

```json
{
  "sourceId": "PUMP-2",
  "revision": 1842,
  "capturedAt": 1784800920,
  "online": true,
  "values": {
    "motor.current": {"value": 4.8, "quality": "good", "ageMs": 240},
    "pressure.outlet": {"value": 2.4, "quality": "good", "ageMs": 240},
    "drive.mode": {"value": "AUTO", "quality": "good", "ageMs": 240}
  }
}
```

UI не изменяет snapshot оптимистически. Draft/desired value хранится отдельно
и исчезает или становится confirmed только после результата устройства.

## 2.6. UI View Model

View Model состоит из:

- `context` — источник, installation, revisions;
- `navigation` — экраны и связи;
- `components` — семантические компоненты;
- `bindings` — ссылки на snapshot/events/commands;
- `policies` — системные ограничения;
- `diagnostics` — предупреждения compiler-а.

Компонент не получает функцию или скрипт. Действия являются перечислением:

- `navigate`;
- `set-draft`;
- `submit-command`;
- `create-critical-request`;
- `acknowledge-event`;
- `dismiss`;
- `cancel-request`.

## 2.7. Display Profile

Профиль задаёт аппаратные пределы. Пример часов:

```json
{
  "profileId": "watch.square.256.mono1",
  "shape": "square",
  "width": 256,
  "height": 256,
  "sizeClass": "s",
  "pixelFormat": "mono1",
  "inputs": ["touch", "button-back"],
  "safeArea": {"top": 8, "right": 8, "bottom": 8, "left": 8},
  "minimumTextPx": 9,
  "minimumPrimaryTextPx": 18,
  "minimumHitPx": 44,
  "budgets": {
    "framebufferBytes": 8192,
    "peakRendererRamBytes": 12288,
    "maxDirtyRects": 8,
    "maxFps": 1
  },
  "capabilities": {
    "grayLevels": 2,
    "supportsTouch": true,
    "supportsAnimation": false,
    "supportsSparkline": true,
    "supportsGauge": true
  }
}
```

Круглый профиль обязан иметь собственную safe-area или mask geometry.

## 2.8. Skin package

Skin package содержит:

- manifest и compatibility;
- semantic color roles;
- typography scale;
- spacing scale;
- line weights;
- icon mapping;
- template variants;
- normal/ambient modes;
- bounded resources.

Он не содержит runtime device values. Установка новой шкурки не требует
перекомпиляции firmware, если binary package format и required semantics
совместимы.

## 2.9. Compiled UI package

Для embedded authoring JSON компилируется в бинарный пакет:

```text
header
string table
screen table
component table
binding table
action table
resource table
compatibility table
crc32
optional signature
```

Пакет не содержит:

- native code;
- bytecode общего назначения;
- рекурсию;
- произвольные выражения;
- сетевые адреса назначения команд в UI actions.

Выражения ограничены заранее определёнными predicates: equality, range,
quality, availability, active-event и command phase.

## 2.10. Лимиты первой версии

- navigation depth: 4;
- screens per package: 32;
- components per screen: 24;
- bindings per package: 128;
- actions per component: 2;
- enum choices: 16;
- event actions: 3;
- UTF-8 string: 160 glyphs;
- no cyclic screen ownership;
- no runtime allocation required by embedded parser.

