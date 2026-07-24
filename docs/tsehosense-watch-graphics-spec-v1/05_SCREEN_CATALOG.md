# 05. Каталог экранов

Все кадры находятся в `reference/screens/` и имеют 256 × 256 px. Не
масштабировать их при pixel-diff.

## Главный экран

| Файл | Состояние |
|---|---|
| `preview-square.png` | квадрат normal |
| `preview-round.png` | круг normal |
| `preview-square-ambient.png` | квадрат ambient |
| `preview-round-ambient.png` | круг ambient |

Эталон принятого вида до расширения сценариев:
`reference/atlases/strict-context-watchface-v0.1.1-preview.png`.

## Контекст устройств

| Файл | Экран |
|---|---|
| `flow/01-devices-square.png` | 3 устройства рядом |
| `flow/02-overview-square.png` | сводка AE-12 |
| `flow/03-parameters-square.png` | таблица параметров |
| `flow/04-devices-round.png` | круглый список |
| `flow/05-overview-round.png` | круглая сводка |
| `flow/06-parameters-round.png` | круглая таблица |

Порядок: home → devices → overview → parameters. Кнопка/свайп вправо возвращает
на один шаг.

## Обычные команды SIG-4

| Файл | Экран |
|---|---|
| `commands/overview-*.png` | сводка SIG-4 |
| `commands/control-*.png` | центр управления |
| `commands/threshold-*.png` | числовой редактор |
| `commands/band-*.png` | enum selector |
| `commands/sending-*.png` | отправка |
| `commands/success-*.png` | подтверждено |
| `commands/error-*.png` | устройство отклонило |
| `commands/offline-*.png` | тайм-аут/нет связи |

`*` означает `square` и `round`.

## Защищённые и критические команды PUMP-2

Файлы JPEG в `safety/`:

- `overview-*` — состояние насоса;
- `control-*` — обычное/защищённое/критическое действие;
- `mode-*` — выбор AUTO/MANUAL;
- `guarded-*` — последствия перехода;
- `hold-*` — удержание;
- `critical-*` — только запрос разрешения;
- `waiting-*`, `approved-*`, `denied-*` — внешний контур.

`approved` не содержит кнопки исполнения. Это обязательное визуальное правило.

## События экосистемы

| Файл | Экран |
|---|---|
| `events/ordinary-*.png` | нижняя плашка |
| `events/important-*.png` | важное полноэкранное |
| `events/blocking-*.png` | блокирующее |
| `events/journal-*.png` | журнал четырёх уровней |
| `events/detail-*.png` | детали события |

Фоновое событие не имеет overlay-кадра: оно появляется только строкой журнала.

## Экранные инварианты

- Canvas всегда 256 × 256.
- Фон всегда чистый чёрный.
- Внешняя web-рамка часов не является частью кадра.
- Нижние постоянные кнопки не добавляются поверх экранов: действие находится в
  контексте строки/экрана, а возврат поддерживается физической кнопкой.
- Скроллбар не рисуется; короткий hint допустим только там, где он не
  пересекает круглую маску.
- Persistent control не может пересекать clip.

## Атласы

`reference/atlases/` содержит крупные листы для обзора языка:

- watchface;
- device flow;
- ecosystem events;
- device commands;
- guarded/critical.

Атлас удобен для проверки цельности. Для точных измерений использовать
отдельные 256 × 256 кадры.
