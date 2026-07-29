# Контроллер экранов и взаимодействия

Навигация реализована в `src/app/watch_ui.c` как ограниченный контроллер:
11 перечисленных экранов, стек глубиной 4 и не более 8 hit targets на экран.
Старый двухэкранный StateSmith-прототип удалён, потому что больше не отражал
рабочую модель интерфейса.

## Вход

Драйвер CST816D передаёт координаты в `watch_ui_process_touch()`. Контроллер
преобразует их в семантические действия: переход, возврат, подтверждение,
удержание, создание critical request и acknowledgment события. Координаты не
попадают в модель данных.

Удержание использует монотонную шкалу RTC1:

- начало на `touch-down`;
- progress при сохранении пальца внутри hit target;
- cancel при выходе, отпускании, смене экрана или event preemption;
- complete после 12 тиков RTC1, то есть 1500 мс.

## Навигация

```text
HOME
├── DEVICES ── OVERVIEW ── PARAMETERS ── METRIC
│                         └─ CONTROLS ── COMMAND
│                                      └─ CRITICAL REQUEST
├── EVENTS ── EVENT DETAIL
└── DIAGNOSTIC
```

Блокирующее событие может открыть `EVENT DETAIL` поверх любого текущего
экрана. `BACK` возвращает пользователя на вытесненный экран. Для critical
request разрешены только request/approval outcomes; execute-action отсутствует.

## Добавление экрана

1. Добавить стабильное значение в `watch_ui_screen_t`.
2. Реализовать bounded draw-функцию без рекурсивного layout и allocation.
3. Регистрировать только семантические hit targets размером не менее 44 px.
4. Добавить переход в `dispatch_action()`.
5. Расширить аппаратный сценарий `scripts/verify-ui.ps1`.
6. Проверить build, Flash/RAM, полный маршрут и heartbeat на плате.
