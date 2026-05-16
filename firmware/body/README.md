# StackChan Body Firmware

公式StackChan向けのPlatformIOファームウェア。

Phase 6では、USB Serial経由の身体制御に加えてWAV音声再生、音量制御、
speaking状態、簡易口パクを扱う。

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
    AudioController.hpp
    AudioState.hpp
    BodyState.hpp
    BodyTypes.hpp
    DisplayController.hpp
    FaceController.hpp
    FaceState.hpp
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
      AudioController.cpp
      AudioState.cpp
      BodyState.cpp
      BodyTypes.cpp
      DisplayController.cpp
      FaceController.cpp
      FaceState.cpp
      LedController.cpp
      MotionController.cpp
    protocol/
      CommandHandler.cpp
      CommandParser.cpp
      ProtocolTypes.cpp
      SerialProtocol.cpp
```

## Phase 6 Scope

Phase 6 adds Audio Playback and speaking body state.

Implemented:

- AudioController
- AudioState
- WAV transfer
- WAV playback
- Audio status
- Volume control
- Speaking state
- Simple mouth animation
- BodyController
- DisplayController
- FaceController
- FaceState
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
- AUDIO:STATUS
- AUDIO:VOLUME:<volume>
- AUDIO:STOP
- AUDIO:WAV:<size>
- FACE expression cleanup
- Formal FACE expressions:
  - neutral
  - happy
  - sad
  - angry
  - sleepy
  - doubt
- Removed from FACE:
  - thinking
  - alert
- Face rendering with M5Stack-Avatar
- Expression rendering
- Automatic blinking
- Gaze state
- Mouth open state
- Speaking state
- Error responses
- Serial protocol documentation

Not implemented:

- Wi-Fi
- HTTP API
- WebSocket
- Bridge application
- Camera
- Microphone
- Input events
- External application integration
- TTS
- STT
- Wake Word
- Conversation handling

## Architecture

```text
main.cpp
  |
  v
BodyController
  |-- BodyState
  |-- AudioController
  |-- DisplayController
  |-- FaceController
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

`FaceController` は顔描画ライブラリへの依存を内部に閉じ込める。
`CommandHandler` や `main.cpp` は、顔の描画詳細を知らずに
`BodyController.setExpression()` だけを呼ぶ。

`M5Stack-Avatar` は内部で描画タスクを起動する。Phase 4では、
`avatar.init()` 直後に `setExpression()` を続けて呼ぶと実機で
FreeRTOS assertが発生したため、起動時はAvatarの初期状態に任せ、
表情変更は `FACE` コマンド経由で行う。

`MotionController` はサーボ値を安全範囲へ clamp する。StackChan-BSPの
角度単位は `10 = 1度`。現在の安全範囲は `x=-450..450`、
`y=0..450` としている。範囲外の値が指定された場合は、適用値を
安全範囲内に丸めてシリアルログへ警告を出す。

`SerialProtocol` は通常コマンドを1行単位で受け取り、レスポンスを `OK` / `ERR`
で返す。`AUDIO:WAV:<size>` だけは `READY AUDIO WAV` の後に指定バイト数の
WAVバイナリを受信する。デバッグログは `[` で始まるため、Bridgeでは区別しやすい。

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
[BOOT] StackChan Body Firmware Phase 6
[BOOT] Initializing body controller
[DISPLAY] begin
[FACE] begin
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
FACE:neutral
FACE:happy
FACE:sad
FACE:angry
FACE:sleepy
FACE:doubt
FACE:thinking
FACE:alert
LED:calm
POSE:neutral
MOVE:0:0
RESET
HELP
```

期待されるレスポンス例:

```text
OK PONG
OK VERSION firmware=0.6.0 protocol=0.1.0 board=stackchan-cores3
OK STATUS mode=Ready expression=Neutral mood=Calm pose=Neutral x=0 y=0 gazeX=0 gazeY=0 speaking=false blinking=false
OK FACE neutral
OK FACE happy
OK FACE sad
OK FACE angry
OK FACE sleepy
OK FACE doubt
ERR INVALID_ARGUMENT expression=thinking
ERR INVALID_ARGUMENT expression=alert
OK LED calm
OK POSE neutral
OK MOVE x=0 y=0
OK RESET
OK HELP commands=PING,VERSION,HELP,STATUS,FACE,LED,POSE,MOVE,RESET,AUDIO expressions=neutral,happy,sad,angry,sleepy,doubt moods=calm,active,speaking,warning,off poses=neutral,look_left,look_right,look_up,look_down audio=AUDIO:STATUS,AUDIO:VOLUME,AUDIO:STOP,AUDIO:WAV
```

目視で確認すること:

- 起動後に顔が表示される
- `FACE` コマンドで顔が変わる
- 自動瞬きが見える
- `FACE:thinking` と `FACE:alert` が `ERR INVALID_ARGUMENT` になる
- `STATUS` に `gazeX` / `gazeY` / `speaking` / `blinking` が含まれる
- `AUDIO:STATUS` で音声状態が返る
- `AUDIO:VOLUME:180` で音量を設定できる
- Bridgeの `/play-wav` 経由でWAV再生中に口が動く
