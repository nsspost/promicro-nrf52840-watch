# Полноценный GUI часов на тестовых данных

Дата проверки: 2026-07-24

## Реализовано

На реальной плате работают nRF52840, круглый GC9A01 240×240 RGB565 и
сенсорный контроллер CST816D. GUI построен поверх framebuffer-free backend
NOG_C, не использует динамическую память и получает данные только из
фиксированной тестовой View Model.

Доступны 11 экранов:

1. `HOME_CONTEXT`;
2. список устройств;
3. обзор устройства;
4. список параметров;
5. деталь параметра со sparkline;
6. список действий;
7. lifecycle guarded-команды;
8. lifecycle critical request;
9. журнал событий;
10. деталь события;
11. диагностика часов.

Тестовый набор содержит PUMP-2, offline-обнаружитель и локальные часы,
quality-состояния `good`, `uncertain`, `stale`, `offline`, а также обычное,
важное и блокирующее события. Внешний транспорт, BLE и реальные данные
намеренно не подключены.

## Safety-поведение

- guarded-команда отправляется только после удержания 1500 мс;
- отображаются отдельные результаты `success`, `rejected` и `timeout`;
- desired state не подменяет confirmed state;
- critical action является только запросом;
- `approved` явно не означает `executing`;
- блокирующее событие вытесняет экран команды или запроса;
- смена экрана отменяет активное удержание;
- минимальная интерактивная зона составляет 44×44 px.
- компактный bounded-шрифт поддерживает русские uppercase UTF-8 подписи.

## Архитектурная граница

```text
test View Model -> bounded watch templates -> semantic hit targets
                                              |
RTC1 -> lifecycle controller -----------------+
                                              v
                                            NOG_C
                                              |
                                              v
                                           GC9A01
```

`ui_demo_model.c` является только сценарием-прототипом. Он не изображает
реальную связь и позже должен быть заменён адаптером над нормализованной
Universal UI View Model без изменения renderer-а.

## Проверка

```powershell
npm run build
npm run flash
npm run verify-ui
npm run verify-running
```

`verify-ui` через mailbox в RAM выполняет 43 действия в обычном контексте
main loop. Сценарий обходит все экраны, проверяет back-navigation, все исходы
guarded/critical lifecycle, event precedence и подтверждение события.

Последняя измеренная сборка:

- Flash text: 20 324 bytes;
- static RAM: 1 604 bytes;
- framebuffer и `malloc` отсутствуют;
- в образ не попадают 64-битное деление, `memcpy` и `memmove`.

## Следующие продуктовые этапы

- заменить тестовую View Model скомпилированным Universal UI package;
- расширить локализацию за пределы встроенного русского demo-набора;
- подключить реальный snapshot/event/command transport;
- реализовать storage, reconnect, TTL/idempotency и authority protocol;
- измерить redraw latency, энергопотребление и читаемость на улице.
