# План интеграции с Gadgetbridge

Дата: 2026-07-27  
Статус: этапы 0–2 реализованы; собственный GATT transport и TIME_SET собраны,
ожидается сквозной тест с телефоном

## 1. Решение

Целевым телефонным приложением для часов является Gadgetbridge. Отдельное
приложение-компаньон для часов сейчас не создаётся.

Chronos не является целевым протоколом. Выполненный Chronos bring-up доказал
работоспособность BLE peripheral, Nordic UART Service, GATT discovery,
подписки и двунаправленной передачи данных, но прикладные пакеты Chronos в
модель часов не интегрированы. Этот код сохраняется только как диагностическое
свидетельство и источник регрессионных тестов BLE.

Целевой набор возможностей первого релиза:

1. синхронизация времени и часового пояса;
2. состояние BLE-соединения;
3. передача в телефон уровня и состояния батареи часов;
4. получение выбранных пользователем уведомлений;
5. метаданные и состояние музыки;
6. управление музыкой с часов;
7. интеграция Техносенс отложена до отдельного решения о маршруте и доверии.

Фитнес, сон, погода, контакты, звонки, быстрые ответы, навигация, установка
приложений и OTA через Gadgetbridge в MVP не входят.

## 2. Что подтверждено исследованием Gadgetbridge

Исследована официальная ветка Gadgetbridge на ревизии `4cc618e` от
2026-07-27, прежде всего реализации Bangle.js и wasp-os.

- Gadgetbridge не имеет универсального протокола устройства. Для новых часов
  требуются собственные `DeviceType`, `DeviceCoordinator` и `DeviceSupport`.
- Текущий NUS часов использует те же UUID и те же физические направления, что
  Bangle.js/wasp-os:
  - service `6e400001-b5a3-f393-e0a9-e50e24dcca9e`;
  - phone-to-watch write `6e400002-b5a3-f393-e0a9-e50e24dcca9e`;
  - watch-to-phone notify `6e400003-b5a3-f393-e0a9-e50e24dcca9e`.
- Gadgetbridge уже предоставляет device-support callback-и для времени,
  уведомлений, удаления уведомлений, состояния музыки и метаданных трека.
- С часов Gadgetbridge принимает события батареи и команды музыки.
- Фильтрация приложений и уведомлений является ответственностью Gadgetbridge;
  часы получают уже разрешённый пользователем поток.
- Обнаружение устройства определяется scan filter, именем и coordinator-ом.
  Одного наличия NUS и имени `Tseho Watch` недостаточно, чтобы штатный
  Gadgetbridge выбрал Bangle.js support.
- Bangle.js использует строковый JSON/JavaScript-подобный протокол. Он удобен
  для smoke-теста, но его полный парсер и Espruino-команды не подходят как
  компактный долгосрочный контракт nRF52840.
- Поддержка нового устройства принимается upstream при наличии документации,
  тестов и человека, готового сопровождать её.

Официальные исходные материалы:

- <https://gadgetbridge.org/internals/topics/support/>
- <https://gadgetbridge.org/internals/development/new-gadget/>
- <https://gadgetbridge.org/internals/development/project-overview/>
- <https://codeberg.org/Freeyourgadget/Gadgetbridge>

## 3. Целевая архитектура

```text
Android notification/media/time APIs
                |
                v
     Gadgetbridge Tseho Watch support
                |
          Tseho Link over BLE
                |
                v
        watch_phone_transport
                |
        watch_phone_protocol
                |
                v
     bounded normalized phone model
          |                  |
          v                  v
    StateSmith/UI       watch commands
```

Границы слоёв:

- BLE transport знает GATT, MTU, connect, subscribe, write и notify, но не
  знает уведомления и музыку.
- Протокол знает кадры, версии и типы сообщений, но не рисует GUI.
- Нормализованная модель имеет фиксированные пределы и не зависит от
  Gadgetbridge, Chronos или Android.
- GUI читает snapshot модели и получает семантические события.
- NOG_C ничего не знает о телефоне и протоколах.

## 4. Протокол Tseho Link v0

До написания Gadgetbridge-кода необходимо принять отдельный ADR протокола.
Предварительные требования:

- отдельный 128-bit service UUID для однозначного discovery;
- RX write/write-without-response и TX notify;
- стандартные Device Information Service и Battery Service там, где это
  уменьшает специальный код;
- бинарный кадр с magic, major/minor version, type, flags, sequence и length;
- явная фрагментация независимо от фактического ATT MTU;
- ограниченный максимальный размер сообщения;
- capability handshake: часы и телефон объявляют поддерживаемые функции;
- подтверждения только для операций, где важен результат;
- защита от duplicate, replay в пределах сессии, malformed и overflow;
- pairing/bonding до передачи содержимого уведомлений;
- little-endian числа и UTF-8 с сокращением только по границе code point;
- golden vectors, одинаковые для C и Java/Kotlin;
- отсутствие heap и неограниченных строк в прошивке.

CRC поверх BLE на первом этапе не обязателен: целостность уже обеспечивает
BLE, а кадр проверяет magic, version и length. Решение фиксируется в ADR после
анализа фрагментации и требований к будущим не-BLE транспортам.

Предлагаемые группы сообщений:

```text
SESSION      HELLO, CAPABILITIES, READY, ERROR
TIME         TIME_SET
BATTERY      WATCH_BATTERY
NOTIFICATION NOTIFY_ADD, NOTIFY_REMOVE
MEDIA        MEDIA_INFO, MEDIA_STATE, MEDIA_VOLUME, MEDIA_COMMAND
```

Для Техносенс пока не резервируются message type или capability. Их нельзя
закрепить до отдельного ADR источника данных, маршрута и модели доверия.

## 5. Точные границы MVP

### Время

Телефон передаёт:

- UTC Unix time;
- смещение UTC в минутах;
- признак 12/24 часа;
- sequence и момент синхронизации.

Часы продолжают идти локально после disconnect. Повторная синхронизация
выполняется после READY, изменения времени/часового пояса и reconnect.

### Соединение

`OFF`, `ADVERTISING`, `CONNECTED`, `SECURED`, `READY` и `STALE` являются
локальными состояниями часов. Отдельный периодический пакет «мы соединены» не
нужен. UI не объявляет канал готовым до handshake и time sync.

### Батарея часов

Часы передают percent и charging state; voltage добавляется только при наличии
измеренного и откалиброванного значения. До появления реального battery driver
Gadgetbridge не должен получать вымышленный процент.

### Уведомления

Gadgetbridge выполняет выбор приложений. Часы получают:

- стабильный notification id;
- категорию;
- краткое имя приложения;
- title и body с фиксированными пределами;
- timestamp;
- add/update/remove.

В часах хранится фиксированная очередь последних восьми уведомлений. Emoji и
неподдерживаемые glyph-и не должны повреждать UTF-8 или разметку.

### Музыка

Телефон передаёт:

- play/pause/stop;
- artist, album и track;
- duration и position, если доступны;
- volume, если доступен.

Часы отправляют:

- play/pause toggle;
- previous/next;
- volume up/down.

Передача аудио на часы не входит в задачу.

## 6. План реализации

### Этап 0 — сохранить baseline и убрать ложные ожидания

Статус: готово.

1. Оставить `BLE_CHRONOS_BRINGUP.md` как журнал аппаратной проверки.
2. Не развивать Chronos framing/parser/model.
3. Переименовать Chronos-специфические состояния диагностики в нейтральные BLE
   состояния.
4. Сохранить текущую рабочую прошивку и RAM diagnostics как baseline.

Готово, когда сборка и прошивка не требуют приложения Chronos для локальной
работы часов.

### Этап 1 — ADR и независимый codec

Статус: готово. Принят
[`ADR-0001-TSEHO-LINK-V0.md`](adr/ADR-0001-TSEHO-LINK-V0.md), C codec подключён
к firmware, golden vectors и проверки границ проходят.

1. Создать `docs/adr/TSEHO_LINK_V0.md`.
2. Зафиксировать UUID, frame header, fragmentation, limits и capabilities.
3. Создать `protocol/tseho-link-v0/` с golden vectors.
4. Реализовать host C encoder/decoder с фиксированными буферами.
5. Проверить split в каждой позиции, duplicate, overflow, malformed UTF-8 и
   неизвестные типы.

Готово, когда один набор vectors проходит C-тесты и может быть реализован на
Android без знания firmware internals.

### Этап 2 — минимальный Gadgetbridge development baseline

Статус: Android-код, unit tests и APK готовы в отдельной локальной ветке.
Инструкция: [`GADGETBRIDGE_DEVELOPMENT.md`](GADGETBRIDGE_DEVELOPMENT.md).

Это не новое приложение. Используется обычный исходный код Gadgetbridge с
небольшим device-support модулем.

1. Создать отдельный fork/ветку Gadgetbridge только перед началом Android-кода.
2. Собрать неизменённый Gadgetbridge локально и установить development APK.
3. Добавить:
   - `TsehoWatchConstants`;
   - `TsehoWatchCoordinator`;
   - `TsehoWatchDeviceSupport`;
   - `DeviceType.TSEHO_WATCH`;
   - имя, иконку и минимальные device settings.
4. Discovery выполнять по Tseho service UUID и имени `Tseho Watch`.
5. Объявлять только реально реализованные возможности.
6. Добавить unit tests codec-а и coordinator-а.

Осталось подтвердить на телефоне discovery, subscription и handshake. Bonding
временно вынесен в отдельный этап безопасности: текущая лабораторная прошивка
его ещё не сохраняет.

### Этап 3 — соединение и время

Статус: собственный service UUID, RX/TX, потоковый parser, `HELLO`, `READY` и
`TIME_SET` реализованы и собираются. Требуется аппаратный сквозной тест.

1. В прошивке выделить transport, protocol и normalized model.
2. Реализовать lifecycle `ADVERTISING -> CONNECTED -> SECURED -> READY`.
3. Реализовать `TIME_SET`.
4. Обновить GUI online/stale state.
5. Проверить reboot телефона, reboot часов, Bluetooth off/on и смену timezone.

Готово, когда время и смещение восстанавливаются после reconnect, а локальные
часы не останавливаются offline.

### Этап 4 — реальная батарея

1. Идентифицировать battery measurement hardware.
2. Реализовать калиброванное измерение и charging state.
3. Добавить Battery Service или `WATCH_BATTERY`.
4. Ограничить частоту обновлений и проверить расход энергии.

Готово, когда Gadgetbridge показывает реальный уровень, а не тестовое значение.

### Этап 5 — фильтрованные уведомления

1. Реализовать Gadgetbridge callback-и add/update/remove.
2. Добавить bounded UTF-8 codec.
3. Добавить очередь из восьми записей и StateSmith event precedence.
4. Проверить фильтры Gadgetbridge, русский текст, длинные строки, emoji,
   duplicate и burst из 100 уведомлений.

Готово, когда часы получают только разрешённые пользователем приложения и
восстанавливают предыдущий экран после уведомления.

### Этап 6 — музыка

1. Подключить Gadgetbridge `onSetMusicInfo` и `onSetMusicState`.
2. Реализовать экран Now Playing.
3. Реализовать семантические команды часов.
4. Проверить несколько Android media players, отсутствие metadata и reconnect.

Готово, когда play/pause/next/previous работают, а экран корректно переживает
смену трека и потерю соединения.

### Этап 7 — upstream

1. Опубликовать спецификацию Tseho Link.
2. Подготовить clean-room описание устройства и protocol documentation.
3. Прогнать Gadgetbridge lint/tests и аппаратную матрицу.
4. Раскрыть использование AI согласно contribution policy.
5. Отправить поддержку устройства upstream и принять обязательство
   сопровождать её.

До принятия upstream используется development APK Gadgetbridge. Это всё тот же
Gadgetbridge, а не отдельное приложение-компаньон.

### Этап 8 — канал Техносенс

Перед реализацией принять отдельный ADR:

1. Откуда телефон получает события: Android notification, локальный BLE-шлюз
   или сеть Техносенс.
2. Нужен ли интернет в сборке Gadgetbridge.
3. Кто является authority и как выполняется authentication.
4. Формат event id, severity, TTL, acknowledge и deduplication.
5. Допустима ли upstream-интеграция или нужен поддерживаемый сервис-провайдером
   flavor/fork Gadgetbridge.

Если события Техносенс уже публикуются Android-приложением как notifications,
первый вариант заработает через обычный фильтр без специального сетевого кода.
Полноценный семантический канал реализуется отдельно и не маскируется под
обычное уведомление.

## 7. Одноразовый Bangle.js smoke-test

Допустим короткий тест до готовности собственного Gadgetbridge support:

- временное имя, распознаваемое Bangle.js coordinator-ом;
- существующий NUS;
- минимальное распознавание времени, notification и music JSON;
- команда музыки с часов обратно в Gadgetbridge.

Этот тест отвечает только на вопрос «проходит ли нужный поток end-to-end».
Его Espruino-команды, имя устройства и JSON parser не входят в Tseho Link и
удаляются после прохождения этапа 2.

## 8. Основные риски

- Gadgetbridge — Android-only решение.
- До upstream merge потребуется development APK.
- Новый device-support требует долгосрочного сопровождающего.
- Стандартная сборка Gadgetbridge не должна неявно становиться клиентом
  внешнего облачного сервиса.
- Уведомления содержат персональные данные, поэтому открытый NUS без bonding
  неприемлем для повседневной версии.
- Музыкальные metadata и управление различаются между Android players.
- UTF-8 и fragmentation способны увеличить RAM, если не закрепить пределы до
  реализации.

## 9. Ближайшая задача

Не писать экраны уведомлений и не переносить Chronos parser. Следующий шаг:

1. принять ADR `Tseho Link v0`;
2. определить точные лимиты RAM/Flash и максимальные payload;
3. создать общие golden vectors;
4. проверить локальную сборку неизменённого Gadgetbridge;
5. только затем параллельно реализовать C codec и Android device support.
