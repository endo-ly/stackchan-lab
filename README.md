# StackChan Body Firmware

## Overview

公式StackChan向けの最小PlatformIOファームウェア。
Phase 1では、起動、画面表示、RGB LED、サーボ初期姿勢、シリアルログのみを確認する。

## Requirements

- PlatformIO
- M5Stack公式 StackChan
- USB接続
- StackChan-BSP

## Environment Notes

Proxmox LXC 内から書き込む場合は、ホスト側の USB シリアルデバイスを
コンテナへ渡す設定が必要。

詳細は [docs/proxmox-lxc-usb.md](docs/proxmox-lxc-usb.md) を参照。

このリポジトリの `platformio.ini` では、コンテナ内の標準ポートを
`/dev/stackchan` としている。

## Build

```sh
pio run
```

## Upload

```sh
pio run -t upload
```

明示する場合:

```sh
pio run -t upload --upload-port /dev/stackchan
```

## Monitor

```sh
pio device monitor
```

明示する場合:

```sh
pio device monitor -p /dev/stackchan -b 115200
```

## Phase 1 Scope

- Display boot message
- Turn on RGB LED at low brightness
- Move servo to safe initial pose
- Print serial logs

## Out of Scope

- Wi-Fi
- HTTP API
- WebSocket
- TTS
- STT
- AI integration
- Camera
- Microphone
- OTA
