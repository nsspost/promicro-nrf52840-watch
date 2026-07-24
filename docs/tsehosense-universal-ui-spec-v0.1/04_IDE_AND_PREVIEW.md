# 4. ЦехоЛаб: редактор и предпросмотр

## 4.1. Цель UX

Инженер редактирует не экран конкретных часов, а семантику представления
устройства. IDE сразу показывает, как она раскладывается на выбранных
Display Profiles.

## 4.2. Рабочая область

Рекомендуемая компоновка:

```text
┌ Каталог / дерево ┬──────── Предпросмотр ────────┬ Инспектор ┐
│ Device           │  square | round | panel | web│ Parameter │
│ Parameters       │                              │ Visibility│
│ Capabilities     │       rendered surface       │ Priority  │
│ Events           │                              │ Kind      │
│ Skins            │                              │ Safety    │
├──────────────────┴──────────────────────────────┴───────────┤
│ Problems | Layout diagnostics | Simulated state | Package   │
└─────────────────────────────────────────────────────────────┘
```

Для Theia это отдельный widget/view contribution. Canvas может быть React, но
источником истины остаётся JSON model, а не состояние React-компонентов.

## 4.3. Что редактирует пользователь

Для параметра:

- primary/secondary/details/hidden;
- priority;
- preferred presentation kinds;
- group;
- compact label;
- precision;
- show trend;
- size hint.

Для capability:

- label;
- control type и choices/bounds;
- risk class только в пределах policy;
- consequence text;
- confirmation method;
- online/deferred flags;
- commissioning visibility.

Для устройства:

- рекомендуемую template family;
- порядок групп;
- primary metric limit hints;
- default route;
- allowed skins.

## 4.4. Что IDE вычисляет автоматически

- layout template;
- реальное число видимых параметров;
- typography token;
- line breaks;
- short label fallback;
- presentation fallback;
- safe-area;
- dirty regions;
- estimated RAM/package size;
- unsupported semantic warnings.

Координаты вручную не редактируются в основном режиме. Позже можно добавить
advanced override для конкретного Display Profile, но он должен быть
ограниченным и валидируемым.

## 4.5. Матрица предпросмотра

Минимальные профили:

1. `watch.square.256.mono1`;
2. `watch.round.256.mono1`;
3. `band.rect.480x128.gray4`;
4. `gateway.panel.320x240.rgb565`;
5. `web.desktop.responsive`.

Минимальные состояния:

- normal;
- stale value;
- offline source;
- sensor fault;
- long labels;
- no localized compact label;
- background event;
- ordinary event;
- important event;
- blocking event;
- command sending/success/error/timeout;
- critical waiting/approved/denied;
- low-memory fallback;
- incompatible skin.

## 4.6. Симулятор

IDE хранит `preview-scenarios/*.json`. Сценарий задаёт:

- device snapshot;
- времени/age;
- event;
- command/request phase;
- user locale;
- selected display and skin;
- simulated transport condition.

Сценарий не попадает в firmware package.

## 4.7. Диагностика

Ошибки:

- неизвестная обязательная schema;
- команда без risk class;
- critical capability с прямым submit action;
- write channel без confirmed feedback;
- единица расходится с channel schema;
- component превышает bounded limits;
- нет fallback template.

Предупреждения:

- primary parameters не помещаются;
- слишком длинная compact label;
- preferred kind не поддержан частью профилей;
- размер текста приближается к minimum;
- skin не имеет иконки и будет использован fallback;
- estimated package/RAM budget превышен.

## 4.8. Preview fidelity

Reference renderer обязан использовать:

- тот же normalized View Model;
- те же template IDs;
- те же size/spacing tokens;
- тот же font metrics manifest;
- те же fallback rules.

Web preview не должен вручную имитировать embedded экран отдельной разметкой.
Различается только drawing backend.

## 4.9. Skin editor v1

Первая версия редактирует:

- typography scale;
- spacing and line tokens;
- semantic colors/gray levels;
- icon mapping;
- template variant selection;
- normal/ambient;
- asset budgets.

Она не является свободным графическим редактором циферблата. Сначала нужно
стабилизировать package contract; pixel-perfect authoring можно добавить позже.

## 4.10. Команды IDE

Предлагаемые команды:

- `TsehoLab: Validate UI Contracts`;
- `TsehoLab: Open Device Preview`;
- `TsehoLab: Compare Display Profiles`;
- `TsehoLab: Simulate Event`;
- `TsehoLab: Simulate Command Lifecycle`;
- `TsehoLab: Compile UI Package`;
- `TsehoLab: Inspect Renderer Budget`;
- `TsehoLab: Export Preview Evidence`.

