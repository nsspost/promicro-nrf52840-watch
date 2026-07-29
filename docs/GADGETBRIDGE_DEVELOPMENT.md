# Локальная разработка Gadgetbridge для Tseho Watch

Дата: 2026-07-27

Gadgetbridge остаётся отдельным Git-репозиторием и не встраивается в исходники
часов:

```text
external/Gadgetbridge
upstream: https://codeberg.org/Freeyourgadget/Gadgetbridge.git
upstream baseline: 4cc618e
local branch: tseho-watch
local revision: cabedeb
```

`external/` игнорируется основным репозиторием часов. Локальный commit
`cabedeb` ещё не опубликован: перед совместной разработкой понадобится
отдельный fork Gadgetbridge, после чего ветку следует отправить туда.

## Что реализовано

- тип устройства `TSEHO_WATCH`;
- discovery по имени `Tseho Watch` и service UUID Tseho Link;
- RX write и TX notify;
- потоковый Tseho Link decoder, не зависящий от ATT MTU;
- handshake `HELLO -> READY`;
- отправка времени и UTC offset;
- отправка add/remove уведомлений после фильтрации Gadgetbridge;
- отправка metadata и состояния музыки;
- приём реальной батареи часов;
- приём команд управления музыкой;
- unit tests на общих golden frames и split в каждой позиции.

Bonding временно выключен с обеих сторон. Это допустимо только для первого
лабораторного сквозного теста. Содержимое реальных уведомлений нельзя
использовать вне контролируемого теста до появления persistent bonds и
повторного включения требования безопасности.

## Сборка

Основной способ сборки теперь зафиксирован в скрипте проекта:

```powershell
npm run build:gadgetbridge
npm run install:gadgetbridge
```

Первый скрипт выставляет `JAVA_HOME` для JDK 21+ и `ANDROID_HOME`, запускает
тест Tseho Link и создаёт development APK. Второй устанавливает уже собранный
APK через `adb`. Portable Temurin 17 в этой среде лежит в
`D:\devtools\temurin21`; системная Java 8 для этой сборки не используется.

Локально установлены portable Temurin 21 и Android SDK. Из каталога
`external/Gadgetbridge`:

```powershell
$env:JAVA_HOME = `
  'D:\workspace_2.5.0\promicro_nrf52840_watch\external\toolchains\temurin-21\jdk-21.0.11+10'
$env:ANDROID_HOME = "$env:LOCALAPPDATA\Android\Sdk"
$env:ANDROID_SDK_ROOT = $env:ANDROID_HOME

.\gradlew.bat `
  testMainlineDebugUnitTest `
  --tests 'nodomain.freeyourgadget.gadgetbridge.service.devices.tsehowatch.TsehoLinkCodecTest' `
  assembleMainlineDebug `
  --no-daemon
```

Проверенный APK:

```text
external/Gadgetbridge/app/build/outputs/apk/mainline/debug/app-mainline-debug.apk
size:   45,896,284 bytes
sha256: 17B31FB5B7693112023EAE75A07DCD8B369A6B244739777757A3F479BC9CD562
```

Этот APK не установлен автоматически: установка меняет состояние телефона и
может конфликтовать с уже установленной сборкой Gadgetbridge.

## Первый сквозной тест

1. Прошить часы свежим `build/watch_firmware.hex`.
2. Установить development APK вручную или через `adb install`.
3. В Gadgetbridge добавить `Tseho Watch`.
4. Проверить в RAM:
   - `ble_state == WATCH_BLE_STATE_READY`;
   - `ble_tx_notifications >= 1`;
   - растут `ble_received_packets` и `ble_received_bytes`.
5. Изменить время/часовой пояс телефона и проверить часы.
6. Только после этого подключать модель уведомлений и экран музыки в firmware.
