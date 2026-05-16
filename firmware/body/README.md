# StackChan Body Firmware

公式StackChan向けのPlatformIOファームウェア。

Phase 3では、Phase 2で整理したController構造の上にUSB Serialの
テキストプロトコルを追加し、PCからStackChanを外部制御できる
状態にする。

## Requirements

- PlatformIO
- M5Stack公式 StackChan
- USB接続
- StackChan-BSP

## Project Layout

```text
firmware/body/
  platformio.ini
  README.md
  include/body/
    BodyController.hpp
    BodyState.hpp
    BodyTypes.hpp
    DisplayController.hpp
    LedController.hpp
    MotionController.hpp
  include/protocol/
    CommandHandler.hpp
    CommandParser.hpp
    ProtocolTypes.hpp
    SerialProtocol.hpp
  src/
    main.cpp
    body/
      BodyController.cpp
      BodyState.cpp
      BodyTypes.cpp
      DisplayController.cpp
      LedController.cpp
      MotionController.cpp
    protocol/
      CommandHandler.cpp
      CommandParser.cpp
      ProtocolTypes.cpp
      SerialProtocol.cpp
```

## Phase 3 Scope

Phase 3 adds a USB Serial text protocol for external control.

Implemented:

- BodyController
- DisplayController
- LedController
- MotionController
- BodyState
- Safe servo range clamp
- Boot sequence logs
- SerialProtocol
- CommandParser
- CommandHandler
- PING
- VERSION
- HELP
- STATUS
- FACE:<expression>
- LED:<mood>
- POSE:<pose>
- MOVE:<x>:<y>
- RESET
- Error responses
- Serial protocol documentation

Not implemented:

- Wi-Fi
- HTTP API
- WebSocket
- Bridge application
- Audio playback
- Camera
- Microphone
- Input events
- External application integration

## Architecture

```text
main.cpp
  |
  v
BodyController
  |-- BodyState
  |-- DisplayController
  |-- LedController
  `-- MotionController

main.cpp
  |
  v
SerialProtocol
  |-- CommandParser
  `-- CommandHandler
        |
        v
      BodyController
```

`main.cpp` は詳細な身体制御やコマンド解析を持たず、`BodyController` と
`SerialProtocol` の `begin()` / `update()` を呼ぶだけに近い形にしている。

`MotionController` はサーボ値を安全範囲へ clamp する。範囲外の値が
指定された場合は、適用値を安全範囲内に丸めてシリアルログへ警告を出す。

`SerialProtocol` は1行単位でコマンドを受け取り、レスポンスを `OK` / `ERR`
で返す。デバッグログは `[` で始まるため、後続のBridge実装では区別しやすい。

プロトコル仕様は [../../docs/protocol/serial-v0.md](../../docs/protocol/serial-v0.md) を参照。

## Environment Notes

Proxmox LXC 内から書き込む場合は、ホスト側の USB シリアルデバイスを
コンテナへ渡す設定が必要。

詳細は [../../docs/proxmox-lxc-usb.md](../../docs/proxmox-lxc-usb.md) を参照。

このプロジェクトの `platformio.ini` では、コンテナ内の標準ポートを
`/dev/stackchan` としている。

## Build

```sh
cd firmware/body
pio run
```

## Upload

```sh
cd firmware/body
pio run -t upload
```

明示する場合:

```sh
pio run -t upload --upload-port /dev/stackchan
```

## Monitor

```sh
cd firmware/body
pio device monitor -p /dev/stackchan -b 115200
```

## Expected Boot Log

```text
[BOOT] StackChan Body Firmware Phase 3
[BOOT] Initializing body controller
[DISPLAY] begin
[LED] begin
[MOTION] begin
[MOTION] reset to neutral
[BODY] mode=Ready expression=Neutral mood=Calm pose=Neutral
[BOOT] Ready
[PROTO] Serial protocol ready
```

## Serial Commands

シリアルモニタで以下を1行ずつ入力する。

```text
PING
VERSION
STATUS
FACE:thinking
LED:calm
POSE:neutral
MOVE:0:0
RESET
HELP
```

期待されるレスポンス例:

```text
OK PONG
OK VERSION firmware=0.3.0 protocol=0.1.0 board=stackchan-cores3
OK STATUS mode=Ready expression=Neutral mood=Calm pose=Neutral x=0 y=0
OK FACE thinking
OK LED calm
OK POSE neutral
OK MOVE x=0 y=0
OK RESET
OK HELP commands=PING,VERSION,HELP,STATUS,FACE,LED,POSE,MOVE,RESET
```
