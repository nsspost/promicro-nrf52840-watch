# ADR-0001: Tseho Link v0

Дата: 2026-07-27  
Статус: принято для первого прототипа

## Контекст

Часам нужен компактный открытый протокол связи с Gadgetbridge. Chronos
подтвердил работоспособность BLE/NUS, но его прикладной протокол не стал частью
часов. Протокол Bangle.js позволяет быстро проверить Gadgetbridge, однако
строковый JSON и Espruino-команды не подходят как постоянный контракт
ограниченного C runtime.

Первый протокол должен поддерживать только:

- handshake и capability negotiation;
- время и UTC offset;
- батарею часов;
- добавление и удаление уведомлений;
- состояние, metadata и команды музыки.

Интеграция Техносенс намеренно исключена. Для неё пока не выделяются opcode,
capability или payload: сначала необходимо выбрать источник и модель доверия.

## Решение

Используется бинарный little-endian протокол `Tseho Link`, версия `0.1`.
Транспортом первого релиза является BLE GATT, но frame codec не зависит от BLE.

### GATT

Для целевого режима выделяется собственная vendor-specific база:

```text
base:    7a5c0000-34f7-4e8b-a2d1-6c91f0b5732e
service: 7a5c0001-34f7-4e8b-a2d1-6c91f0b5732e
RX:      7a5c0002-34f7-4e8b-a2d1-6c91f0b5732e
TX:      7a5c0003-34f7-4e8b-a2d1-6c91f0b5732e
```

- RX: Gadgetbridge -> часы, write и write without response;
- TX: часы -> Gadgetbridge, notify;
- advertising содержит service UUID и имя `Tseho Watch`;
- Device Information Service и Battery Service добавляются отдельно;
- содержимое уведомлений разрешается только после pairing/bonding.

Существующий Nordic UART Service остаётся только для переходного smoke-test и
диагностики. Он не является UUID целевого протокола.

### Frame header

Размер header — 10 байт:

| Offset | Size | Поле |
| ---: | ---: | --- |
| 0 | 1 | magic `0x54` (`T`) |
| 1 | 1 | magic `0x4C` (`L`) |
| 2 | 1 | major version, сейчас `0` |
| 3 | 1 | minor version, сейчас `1` |
| 4 | 1 | message type |
| 5 | 1 | flags |
| 6 | 2 | sequence, little-endian |
| 8 | 2 | payload length, little-endian |

Максимальный payload v0.1 — 384 байта, полный frame — 394 байта.

Флаги:

- bit 0 `ACK_REQUIRED`;
- bit 1 `RESPONSE`;
- остальные биты должны быть нулевыми в v0.1.

BLE write/notify может содержать часть frame, один frame или несколько frames.
Потоковый decoder не зависит от ATT MTU, собирает frame по `payload length` и
восстанавливается после мусора или oversized header.

CRC в v0.1 отсутствует: BLE уже проверяет целостность пакетов. Если Tseho Link
будет перенесён на транспорт без такой гарантии, CRC оформляется новой minor
версией или transport envelope, а не молча добавляется в существующий frame.

### Message types

```text
0x01 HELLO
0x02 READY
0x03 ACK
0x7F ERROR

0x10 TIME_SET

0x20 WATCH_BATTERY

0x30 NOTIFY_ADD
0x31 NOTIFY_REMOVE

0x40 MEDIA_INFO
0x41 MEDIA_STATE
0x42 MEDIA_VOLUME
0x43 MEDIA_COMMAND
```

Неизвестный type не ломает framing. После handshake он либо игнорируется, либо
получает `ERROR_UNSUPPORTED_TYPE`, если был установлен `ACK_REQUIRED`.

### Capabilities

```text
bit 0 TIME
bit 1 WATCH_BATTERY
bit 2 NOTIFICATIONS
bit 3 MEDIA_INFO
bit 4 MEDIA_CONTROL
```

Незаявленная capability не должна использоваться. Новые возможности получают
новый bit только после спецификации payload и поведения при downgrade.

### Payload v0.1

`HELLO`, 8 байт:

| Поле | Тип |
| --- | --- |
| capabilities | `u32` |
| max_payload | `u16` |
| max_chunk | `u16` |

`READY`: пустой payload.

`ACK`, 4 байта:

| Поле | Тип |
| --- | --- |
| acknowledged_sequence | `u16` |
| status | `u8` |
| acknowledged_type | `u8` |

`TIME_SET`, 8 байт:

| Поле | Тип |
| --- | --- |
| UTC Unix seconds | `u32` |
| UTC offset minutes | `i16` |
| hour format | `u8`, `0=12h`, `1=24h` |
| reserved | `u8`, должен быть `0` |

32-bit Unix seconds не требуют 64-bit деления и покрывают даты до 2106 года.

`WATCH_BATTERY`, 4 байта:

| Поле | Тип |
| --- | --- |
| percent | `u8`, `0..100`, `0xFF=unknown` |
| flags | `u8`, bit 0 charging |
| millivolts | `u16`, `0=unknown` |

`NOTIFY_ADD`:

```text
u32 id
u32 UTC Unix seconds
u8  category
u8  app_length
u8  title_length
u16 body_length
u8  text[app_length + title_length + body_length]
```

Пределы: app 24 байта, title 64 байта, body 240 байт. Строки UTF-8 без NUL,
сокращаются только по границе code point.

`NOTIFY_REMOVE`, 4 байта: `u32 id`.

`MEDIA_INFO`:

```text
u32 duration_seconds
u32 position_seconds
u8  artist_length
u8  album_length
u8  track_length
u8  text[artist_length + album_length + track_length]
```

Пределы: artist 48, album 48, track 96 байт.

После трёх строк может присутствовать необязательное поле источника:

```text
u8  source_app_length
u8  source_app[source_app_length]
```

`source_app` — отображаемое Android-имя владельца активной media session,
например `Яндекс Музыка`. Его отсутствие означает fallback `МУЗЫКА`; старые
реализации игнорируют этот хвост payload.

`MEDIA_STATE`, 8 байт:

```text
u8  state       // 0 stopped, 1 playing, 2 paused
u8  shuffle     // 0 off, 1 on, 0xFF unknown
u8  repeat      // 0 off, 1 one, 2 all, 0xFF unknown
u8  reserved
u32 position_seconds
```

`MEDIA_VOLUME`, 1 байт: `0..100`, `0xFF=unknown`.

`MEDIA_COMMAND`, 1 байт:

```text
0 toggle
1 play
2 pause
3 previous
4 next
5 volume_down
6 volume_up
```

### Sequence и ACK

- Sequence увеличивается отдельно в каждом направлении modulo 65536.
- Повторный `(type, sequence)` в одной сессии не применяется второй раз.
- TIME_SET, WATCH_BATTERY и media metadata не требуют ACK.
- Команды с необратимым эффектом в v0.1 отсутствуют.
- `ACK_REQUIRED` используется для handshake и будущих подтверждаемых команд.
- BLE reconnect начинает новую сессию и новый duplicate window.

### Совместимость

- Несовпадающий major означает несовместимый протокол.
- Более новый minor допускается только после HELLO и пересечения capabilities.
- Поля reserved должны отправляться нулевыми и игнорироваться принимающей
  стороной.
- Неизвестные capabilities и message types не меняют существующую семантику.

## Последствия

Плюсы:

- codec не требует heap, JSON, `printf`, 64-bit division или libc memory;
- Gadgetbridge и firmware тестируются одинаковыми vectors;
- новые функции не требуют нового BLE service;
- GUI остаётся независимым от Android.

Цена:

- до принятия upstream нужен development APK Gadgetbridge;
- C и Android codec необходимо сопровождать синхронно;
- pairing/bonding и persistent keys требуют отдельной реализации;
- переходный NUS smoke-test не совместим с целевым wire format.

## Проверки

- encode/decode каждого golden vector;
- каждый возможный split frame;
- несколько frames в одном chunk;
- мусор до magic;
- ложный первый magic byte;
- payload length 384 и overflow 385;
- неизвестные type/flags;
- truncated frame с продолжением;
- UTF-8 boundary tests на уровне payload codec;
- одинаковый результат C и Android implementations.
