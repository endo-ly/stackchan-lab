# StackChan Body Firmware

公式StackChan向けのPlatformIOファームウェア。

Phase 2では、Phase 1の最小ファームウェアをController構造へ分割し、
表示、LED、サーボ、状態管理をファームウェア内部で扱いやすくする。

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
  src/
    main.cpp
    body/
      BodyController.cpp
      BodyState.cpp
      BodyTypes.cpp
      DisplayController.cpp
      LedController.cpp
      MotionController.cpp
```

## Phase 2 Scope

Phase 2では、Phase 1の最小ファームウェアをController構造へ分割する。

Implemented:

- BodyController
- DisplayController
- LedController
- MotionController
- BodyState
- Safe servo range clamp
- Boot sequence logs
- Simple internal demo

Not implemented:

- Wi-Fi
- HTTP API
- WebSocket
- Serial command parser
- Audio playback
- Camera
- Microphone
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
```

`main.cpp` は詳細な身体制御を持たず、`BodyController.begin()` と
`BodyController.update()` を呼ぶだけに近い形にしている。

`MotionController` はサーボ値を安全範囲へ clamp する。範囲外の値が
指定された場合は、適用値を安全範囲内に丸めてシリアルログへ警告を出す。

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
[BOOT] StackChan Body Firmware Phase 2
[BOOT] Initializing body controller
[DISPLAY] begin
[LED] begin
[MOTION] begin
[MOTION] reset to neutral
[BODY] mode=Idle expression=Neutral mood=Calm pose=Neutral
[BOOT] Ready
```

