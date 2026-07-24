# 1. Архитектура универсального renderer-а

## 1.1. Цель

Один набор паспортных данных должен автоматически порождать полезный интерфейс
на дисплеях разного размера, формы, цветности и способа ввода. При этом:

- малые узлы не содержат тяжёлого layout engine;
- layout не зависит от транспорта BLE, LoRa, MQTT или локальной шины;
- web и embedded показывают одинаковый смысл;
- интерфейс деградирует предсказуемо;
- опасные действия остаются безопасными на любом renderer-е.

## 1.2. Пять слоёв

### Слой A — Domain semantics

Источники: protected board identity, паспорт установки, channel schema,
capability schema и event schema.

Слой отвечает на вопросы «что это?» и «что разрешено?». Он не знает
координаты, размер шрифта, форму дисплея и конкретную шкурку.

### Слой B — Runtime state

Содержит:

- подтверждённые значения и revision;
- timestamp и freshness;
- quality: `good`, `uncertain`, `bad`, `stale`, `offline`;
- активные события;
- состояние команд и critical-запросов;
- online/capability availability.

Неподтверждённое локальное значение хранится отдельно от confirmed state.

### Слой C — UI View Model

Формируется на шлюзе, сервере или в IDE. Это транспортно-независимое дерево
экранов и семантических компонентов:

- `device-overview`;
- `metric`;
- `parameter-list`;
- `control-list`;
- `event-journal`;
- `event-detail`;
- `command-status`;
- `critical-request-status`.

View Model содержит приоритеты и bindings, но не абсолютные пиксельные
координаты.

### Слой D — Layout policy

Получает View Model и Display Profile. Выбирает один из ограниченного набора
шаблонов, число видимых элементов, размерные токены и fallback.

Это не CSS и не произвольная рекурсивная разметка. На embedded-цели алгоритм
должен быть bounded и работать без динамической аллокации.

### Слой E — Skin

Содержит:

- шрифтовые токены;
- толщины линий;
- отступы;
- набор иконок;
- варианты шаблонов;
- правила normal/ambient;
- ресурсы.

Skin не содержит channel IDs, MQTT topics, командных адресов и safety policy.

## 1.3. Где выполняются слои

| Компонент | A | B | C | D | E |
|---|---:|---:|---:|---:|---:|
| Простой датчик | минимум | локальное | нет | нет | нет |
| Блок 1.1 | cache | да | опционально | нет | нет |
| Шлюз | да | да | основной compiler | опционально | distribution |
| Часы | cache | да | consume | embedded | embedded |
| Дисплей шлюза | cache | да | consume/local | embedded | embedded |
| Сервер/web | да | да | compiler/consume | web | web |
| ЦехоЛаб | authoring | simulator | reference compiler | reference | editor |

## 1.4. Откуда берётся интерфейс

View Model строится детерминированно:

1. загрузить device profile и channel schemas;
2. объединить с паспортом установки;
3. проверить версии и миграции;
4. привязать live snapshot;
5. вычислить visibility и display priority;
6. добавить доступные capabilities;
7. добавить event/command layers;
8. применить policy конкретного пользователя/renderer-а;
9. выбрать template family;
10. выполнить layout под Display Profile.

Одинаковые входы, версии каталогов и профиль дисплея обязаны давать одинаковую
семантическую структуру.

## 1.5. Template-first вместо свободного метаязыка

Первая версия поддерживает фиксированные семейства:

| Template family | Назначение |
|---|---|
| `home-context` | время, локальный статус, текущий контекст |
| `device-list` | доступные устройства |
| `device-overview` | состояние и главные параметры |
| `parameter-list` | полный список параметров |
| `control-list` | доступные действия |
| `metric-detail` | число, шкала или короткий тренд |
| `event-journal` | журнал |
| `event-detail` | подробности события |
| `command-status` | жизненный цикл команды |
| `critical-request` | запрос внешнего полномочия |

Паспорт может подсказать предпочтительный presentation kind: `number`,
`state`, `lamp`, `gauge`, `bar`, `sparkline`, `enum`, `boolean`, `trigger`.
Renderer вправе заменить его fallback-вариантом, если профиль не поддерживает
предпочтительный вид.

## 1.6. Responsive-модель

Display Profile объявляет не только разрешение, но и возможности:

- `shape`: square, round, rectangular;
- `pixelFormat`: mono1, gray2, gray4, rgb565, rgba;
- `physicalClass`: watch, band, panel, desktop;
- `input`: touch, crown, buttons, keyboard, pointer;
- safe area;
- memory and redraw budgets;
- minimum hit target;
- supported presentation kinds.

Layout policy использует размерные классы:

- `xs`: 128–199 px по короткой стороне;
- `s`: 200–319 px;
- `m`: 320–599 px;
- `l`: 600–1023 px;
- `xl`: 1024 px и выше.

Профиль может переопределить класс вручную. Часы 256×256 относятся к `s`,
панель 3–4" обычно к `m`, desktop/web — `l` или `xl`.

## 1.7. Деградация

При нехватке площади renderer последовательно:

1. убирает secondary annotation;
2. заменяет график числом;
3. заменяет gauge на bar или число;
4. оставляет только primary metrics;
5. переносит остальные параметры в список;
6. сокращает подписи до `compactLabel`;
7. показывает source ID, severity и title как последний fallback.

Renderer не уменьшает основной текст ниже accessibility minimum профиля.

## 1.8. События и команды как независимые слои

Приоритет поверхности:

```text
blocking event
  > important event
    > critical request status
      > command status
        > ordinary event
          > current screen
```

Этот порядок является системным и не меняется шкуркой.

## 1.9. Версии и совместимость

Каждый пакет несёт:

- `format`;
- `formatVersion`;
- `schemaVersion`;
- `catalogVersion`;
- `minRendererVersion`;
- `requiredSemantics`;
- `contentHash`;
- CRC32 для embedded binary;
- необязательную подпись для доверенной distribution.

Неизвестная обязательная семантика делает пакет несовместимым. Неизвестная
необязательная подсказка игнорируется.

