# ProMicro nRF52840 watch

Целевым телефонным companion выбран Gadgetbridge. Chronos использовался только
для проверки BLE/NUS и больше не определяет прикладной протокол часов. План:
[`docs/GADGETBRIDGE_INTEGRATION_PLAN.md`](docs/GADGETBRIDGE_INTEGRATION_PLAN.md).

На подключённой плате работает круглый GUI 240×240 на тестовых данных:
11 экранов, semantic touch, guarded/critical lifecycle, журнал событий и
блокирующее event preemption. На S140 собран собственный BLE GATT service
Tseho Link с bounded C codec, handshake и синхронизацией времени. Отдельная
ветка Gadgetbridge распознаёт `Tseho Watch`, передаёт время, уведомления и
музыку и принимает батарею/команды плеера. Следующий шаг — сквозной тест с
телефоном. Подробности:
[`docs/GADGETBRIDGE_DEVELOPMENT.md`](docs/GADGETBRIDGE_DEVELOPMENT.md).

В прошивке есть две встроенные шкурки: исходная цветная и новая строгая
монохромная. Чтобы переключиться, нажмите на часы в верхней части главного
экрана, затем `СМЕНИТЬ СКИН` и `НАЗАД`. Выбор пока хранится только до
перезагрузки; после reset включается цветная шкурка.

Минимальная безопасная основа прошивки наручных часов и локальное окружение
сборки/отладки через J-LinkOB.

Архитектурные документы:

- [`docs/PROJECT_DESCRIPTION.md`](docs/PROJECT_DESCRIPTION.md);
- [`docs/CURRENT_STATE_AND_ROADMAP.md`](docs/CURRENT_STATE_AND_ROADMAP.md);
- [`docs/BLE_CHRONOS_BRINGUP.md`](docs/BLE_CHRONOS_BRINGUP.md);
- [`docs/CHRONOS_INTEGRATION_PLAN.md`](docs/CHRONOS_INTEGRATION_PLAN.md)
  (архивный);
- [`docs/GADGETBRIDGE_INTEGRATION_PLAN.md`](docs/GADGETBRIDGE_INTEGRATION_PLAN.md);
- [`docs/GADGETBRIDGE_DEVELOPMENT.md`](docs/GADGETBRIDGE_DEVELOPMENT.md);
- [`dependencies.json`](dependencies.json).

## Что уже есть

- bare-metal Cortex-M4F startup и linker script для nRF52840 (1 MiB Flash,
  256 KiB RAM);
- диагностическая прошивка без обращения к GPIO;
- локально закреплённые GCC, CMake, Ninja и резервный OpenOCD;
- SEGGER J-Link Commander/GDB Server для штатного Windows-драйвера J-LinkOB;
- S140 advertising и собственный Tseho Link GATT service;
- C/Java codec с общими golden frames и development APK Gadgetbridge;
- PowerShell-команды для probe/build/flash/GDB;
- задачи VS Code.

Проект пока намеренно не предполагает распиновку дисплея, кнопок, зарядного
контроллера и LED. Неверное предположение о GPIO у разных ProMicro-клонов может
конфликтовать с подключённой периферией.

## Команды

Из PowerShell в каталоге проекта:

```powershell
npx xpm install
npm run install-jlink
npm run probe
npm run build
npm run build:gui
npm run flash
npm run verify-ui
npm run verify-running
npm run render:strict-host
npm run generate:strict-icons
npm run generate:strict-font
```

Для отладки запустите два терминала:

```powershell
npm run debug-server
```

```powershell
npm run gdb
```

В GDB можно проверить работу диагностической прошивки:

```text
monitor reset
continue
<Ctrl+C>
p/x watch_debug_state
```

Поле `heartbeat` должно расти между остановками.

`build:gui` дополнительно проверяет интеграцию с независимым репозиторием
`external/NOG_C`. Universal UI пока подключён на уровне контрактов: его
реализация начинается после стабилизации reference model согласно собственной
спецификации.

## Важные ограничения

`flash` перед первой записью автоматически сохраняет весь Flash и UICR в
`backups/`. Затем она записывает только адреса, занятые
`watch_firmware.hex`, начиная с `0x00000000`. Команда не выполняет mass erase и
не пишет UICR. Однако прошивка с адреса 0 заменяет находившийся там
загрузчик/приложение — храните созданный backup.

OpenOCD оставлен как альтернативный backend. На Windows он не может открыть
J-Link с фирменным SEGGER USB-драйвером (`LIBUSB_ERROR_NOT_SUPPORTED`).
Подменять драйвер через Zadig не рекомендуется; штатные J-Link Commander и GDB
Server работают с ним напрямую.

Перед разработкой функций часов нужно зафиксировать точную модель платы и
распиновку: дисплей, touch/кнопки, вибромотор, RTC, датчики, измерение батареи и
управление питанием.
