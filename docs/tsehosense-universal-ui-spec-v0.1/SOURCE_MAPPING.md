# Связь с существующей архитектурой ЦехоСенс

Этот пакет не заменяет основную документацию. Он развивает уже принятые
решения.

| Существующее решение | Как используется Universal UI |
|---|---|
| Protected supervisor и паспорт установки | доверенный источник identity, schema и revision |
| Реестр sensor types и app bundles | совместимость descriptor-а с физическим устройством |
| Общий app-control и fault events | единые runtime state и event bindings |
| Паспорт collection node | source identity для UI registry |
| MQTT namespace | транспорт snapshot/events, но не часть View Model |
| PLC C-based RTE и process image | bindings capability к desired/confirmed channels |
| IEC ABI и variable map | стабильная связь IDE/PLC/UI |
| ЦехоЛаб на Eclipse Theia | authoring, validation и preview surface |
| Controlled forcing/service mode | отдельный инженерный контур, не UI capability |
| React Flow + независимый Graphical IR | пример правильного разделения editor и source of truth |
| Channel roles | определяют measurement/command/status/diagnostic semantics |
| Reusable channel schemas | базовый словарь параметров и команд |
| Catalog migration manifests | обязательный путь изменения UI metadata |

## Документы-источники, использованные при подготовке

- `README(1).md` — многоуровневая архитектура и поток событий.
- `TECHNICAL_SYSTEM_DESCRIPTION.md` — UI системы и путь данных.
- `LEVEL_1_2_BLOCK_REQUIREMENTS.md` — ограничения нижних уровней.
- `DECISIONS.md` — DEC-009…DEC-036.
- `strict-context-watchface/skin-package/device-flow.json`.
- `strict-context-watchface/skin-package/event-flow.json`.
- `strict-context-watchface/skin-package/command-flow.json`.
- `strict-context-watchface/skin-package/manifest.json`.

## Решения, которые предлагается добавить в основной журнал

1. Universal UI model and bounded renderers — `ADR_PROPOSAL.md`.
2. Binary UI package v1 — отдельный ADR после TypeScript prototype.
3. Skin trust/signature policy — отдельный ADR перед distribution.
4. Critical authority model — отдельный safety ADR до реального управления
   промышленным оборудованием.

