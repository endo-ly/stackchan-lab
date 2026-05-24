# StackChan Body Firmware

公式StackChan向けのPlatformIOファームウェア。

Phase 8では、USB Serial経由の身体制御に加えて、StackChan本体がWi-Fiへ接続し、
BridgeからHTTP over Wi-Fiで制御できる。

## Requirements

- PlatformIO
- M5Stack公式 StackChan
- USB接続
- Wi-Fi 2.4GHz network
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
    InputController.hpp
    InputEvent.hpp
    InputState.hpp
    LedController.hpp
    MotionController.hpp
  include/protocol/
    CommandHandler.hpp
    CommandParser.hpp
    ProtocolTypes.hpp
    SerialProtocol.hpp
  include/network/
    NetworkController.hpp
    WiFiManager.hpp
    HttpServerController.hpp
    NetworkState.hpp
    DeviceAuth.hpp
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
      InputController.cpp
      InputEvent.cpp
      InputState.cpp
      LedController.cpp
      MotionController.cpp
    protocol/
      CommandHandler.cpp
      CommandParser.cpp
      ProtocolTypes.cpp
      SerialProtocol.cpp
    network/
      NetworkController.cpp
      WiFiManager.cpp
      HttpServerController.cpp
      DeviceAuth.cpp
```

## Phase 8 Scope

Phase 8 adds wireless transport.

Implemented:

- Wi-Fi connection
- Saved Wi-Fi configuration in ESP32 Preferences/NVS
- NetworkController
- WiFiManager
- HttpServerController
- DeviceAuth
- HTTP API on StackChan
- Wireless body control
- Wireless audio playback
- Wireless event retrieval
- Serial Wi-Fi setup commands
- mDNS hostname start after Wi-Fi connection

Serial Wi-Fi setup:

- `WIFI:STATUS`
- `WIFI:SET_JSON:<size>`
- `WIFI:CONNECT`
- `WIFI:CLEAR`

HTTP API on StackChan:

- `GET /health`
- `GET /version`
- `GET /status`
- `GET /capabilities`
- `POST /face`
- `POST /led`
- `POST /pose`
- `POST /move`
- `POST /reset`
- `POST /play-wav`
- `GET /audio/status`
- `POST /audio/volume`
- `POST /audio/stop`
- `GET /mic/status`
- `POST /mic/record`
- `GET /events`
- `GET /events/latest`
- `POST /events/clear`
- `GET /wifi/status`
- `POST /wifi/connect`
- `POST /wifi/clear`
- `GET /camera/snapshot` returns 501 in Phase 8

Not implemented:

- STT
- HTTP Wi-Fi credential setup
- WebSocket
- MQTT
- BLE control
- Camera snapshot implementation
- External application integration

## Phase 7 Scope

Phase 7 adds Input Events.

Implemented:

- InputController
- InputEvent
- InputState
- Event queue
- Touch/button events
- Simple IMU events
- Serial event commands
- STATUS input summary
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
- EVENTS
- EVENTS:LATEST
- EVENTS:CLEAR
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
  |-- InputController
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

main.cpp
  |
  v
NetworkController
  |-- WiFiManager
  |-- DeviceAuth
  `-- HttpServerController
        |
        v
      BodyController
```

`main.cpp` は詳細な身体制御やコマンド解析を持たず、`BodyController` と
`SerialProtocol` と `NetworkController` の `begin()` / `update()` を呼ぶだけに近い形にしている。

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

`InputController` はタッチ・ボタン・IMUを監視し、入力変化を
`InputEvent` として固定長キューへ保存する。イベント取得とクリアは分けており、
`EVENTS` で読んだだけではキューを消さない。

`SerialProtocol` は通常コマンドを1行単位で受け取り、レスポンスを `OK` / `ERR`
で返す。`AUDIO:WAV:<size>` だけは `READY AUDIO WAV` の後に指定バイト数の
WAVバイナリを受信する。`EVENTS` は複数行レスポンスになり、最後に
`END EVENTS` を返す。デバッグログは `[` で始まるため、Bridgeでは区別しやすい。

`NetworkController` は起動時にESP32 Preferences/NVSから保存済みWi-Fi設定を読み、
接続に成功した場合だけHTTP serverを起動する。Wi-Fi設定がない、または接続に失敗した場合でも
USB Serialは使えるため、`WIFI:SET_JSON:<size>` や `WIFI:CLEAR` で復旧できる。

HTTP APIはLAN内利用を前提にしている。`authToken` が保存されている場合は、
`X-StackChan-Token` ヘッダが一致するリクエストだけを受け付ける。
`authToken` 未設定の場合は開発モードとして認証なしで通す。

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
[BOOT] StackChan Body Firmware Phase 8
[BOOT] Initializing body controller
[DISPLAY] begin
[INPUT] begin
[FACE] begin
[LED] begin
[MOTION] begin
[MOTION] reset to neutral
[BODY] mode=Ready expression=Neutral mood=Calm pose=Neutral
[BOOT] Ready
[NET] begin
[NET] saved Wi-Fi config not found
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
EVENTS
EVENTS:LATEST
EVENTS:CLEAR
WIFI:STATUS
RESET
HELP
```

期待されるレスポンス例:

```text
OK PONG
OK VERSION firmware=0.8.0 protocol=0.1.0 board=stackchan-cores3
OK STATUS mode=Ready expression=Neutral mood=Calm pose=Neutral x=0 y=0 gazeX=0 gazeY=0 speaking=false blinking=false eventCount=0 latestEvent=none latestTarget=none latestValue=none touchActive=false buttonActive=false imuMoving=false
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
OK EVENTS count=0
OK EVENTS LATEST none
OK EVENTS CLEAR
OK WIFI STATUS connected=false ssid= ip= hostname=stackchan-001 rssi=0 auth=false
OK RESET
OK HELP commands=PING,VERSION,HELP,STATUS,FACE,LED,POSE,MOVE,RESET,AUDIO,EVENTS,WIFI,DEVICE expressions=neutral,happy,sad,angry,sleepy,doubt moods=calm,active,speaking,warning,off poses=neutral,look_left,look_right,look_up,look_down audio=AUDIO:STATUS,AUDIO:VOLUME,AUDIO:STOP,AUDIO:WAV events=EVENTS,EVENTS:LATEST,EVENTS:CLEAR wifi=WIFI:STATUS,WIFI:SET_JSON,WIFI:CONNECT,WIFI:CLEAR device=DEVICE:CONFIG_JSON
```

## Wi-Fi Setup Over USB Serial

Wi-Fi credentialはリポジトリに入れない。実機へUSB Serial経由で一度だけ保存する。

Bridge側の設定CLIを使う。

Bridgeが起動中の場合は、USB Serialを掴んでいるため先に止める。

```bash
pkill -f "tsx src/main.ts"
```

```bash
cd bridge
npm run wifi:setup -- config.yaml
```

CLIはSSID、Wi-Fi password、hostname、authTokenを聞き、passwordとauthTokenは入力中に表示しない。
内部では、CLIが入力内容をJSON化し、バイト数を数えたうえで `WIFI:SET_JSON:<size>` を送り、
続けてJSON本体をUSB Serialで転送する。人間がJSONやサイズを手で扱う必要はない。

authTokenはStackChan本体HTTP API用の合言葉。Wi-Fiパスワードとは別に、ローカルで生成する。

```bash
openssl rand -hex 32
```

tokenを再生成する場合は、新しいtokenを作って `npm run wifi:setup -- config.yaml` を再実行する。
その後、Bridge側のローカル `config.yaml` の `wifi.token` も同じ値へ更新する。

stt-adapter URLなどの運用設定は、Wi-Fi設定とは別に送る。

```bash
npm run device:config -- config.yaml
```

設定を消す場合:

```text
WIFI:CLEAR
OK WIFI CLEAR
```

保存済み設定で再接続する場合:

```text
WIFI:CONNECT
OK WIFI CONNECTED ip=192.168.0.123 hostname=stackchan-001
```

## Device HTTP Smoke Test

Wi-Fi設定後、PC側からStackChan本体へ直接確認する。
上位システムの通常入口はBridgeだが、実機切り分けでは本体HTTPを直接叩く。

```bash
STACKCHAN_IP=192.168.0.123
TOKEN=<device-token>

curl "http://$STACKCHAN_IP/health"
curl "http://$STACKCHAN_IP/status" -H "X-StackChan-Token: $TOKEN"
curl -X POST "http://$STACKCHAN_IP/face" \
  -H "Content-Type: application/json" \
  -H "X-StackChan-Token: $TOKEN" \
  -d '{"expression":"happy"}'
curl "http://$STACKCHAN_IP/events" -H "X-StackChan-Token: $TOKEN"
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
- 画面タップ後に `EVENTS` で `touch screen tap` と `x` / `y` が返る
- 本体を軽く動かした後に `EVENTS` で `imu device moved` または `shake` が返る

## Phase 9.5 Microphone

Phase 9.5 moves audio input toward the StackChan body microphone.

Implemented in this step:

- microphone initialization through M5Unified
- short 16kHz mono 16-bit recording
- in-memory WAV generation
- upload to configured `stt-adapter`
- HTTP status and record endpoints
- microWakeWord dependency foundation
- wake word state model
- HTTP wake status/start/stop endpoints

Not implemented in this step:

- wake-triggered recording

Microphone status:

```bash
curl -H "X-StackChan-Token: <token>" \
  http://<stackchan-ip>/mic/status
```

`POST /mic/record` records a short WAV, sends it to the configured `speechServicesUrl`, and returns the STT response.

Copy-paste friendly test:

```bash
export STACKCHAN_IP="<stackchan-ip>"
export TOKEN="<token>"

curl -H "X-StackChan-Token: $TOKEN" \
  "http://$STACKCHAN_IP/mic/status"

curl -X POST "http://$STACKCHAN_IP/mic/record" \
  -H "X-StackChan-Token: $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"durationMs":3000,"source":"stackchan"}'
```

Expected rough recording sizes for 3 seconds:

```json
{
  "ok": true,
  "data": {
    "recorded": true,
    "uploaded": true,
    "durationMs": 3000,
    "pcmBytes": 96000,
    "wavBytes": 96044,
    "httpStatus": 200
  }
}
```

The recorded WAV is kept in StackChan RAM while it is uploaded. It is not saved as a file on the PC, SD, or flash storage.

`speechServicesUrl` is stored by the device config CLI. The CLI reads Bridge `config.yaml` `stt.transcribe_url`. Re-run device config when changing the STT service endpoint.

```bash
cd /root/workspace/stackchan-lab/bridge
npm run device:config -- config.yaml
```

When `stt-adapter` has a Bridge callback configured, this same `/mic/record` flow also creates a Bridge STT event:

```bash
curl "http://127.0.0.1:8787/stt/latest"
curl "http://127.0.0.1:8787/events"
```

## Phase 9.5 Wake Word

Wake word detection is being implemented with a real on-device microWakeWord-style path.

Current foundation:

- `esp-micro-speech-features`
- `MicroTFLite`
- bundled `okay_nabu` microWakeWord model
- 16kHz mic chunk capture split into 10ms frontend blocks
- TFLite inference loop
- wake-triggered 3-second recording and upload to configured `stt-adapter`
- `WakeState`
- `WakeController`
- `GET /wake/status`
- `POST /wake/start`
- `POST /wake/stop`

The model can be loaded and tensor allocation is verified at boot.
`/wake/start` starts on-device mic streaming and inference.
The current bundled model listens for `Okay Nabu`; it is an engine bring-up model, not the final StackChan Japanese wake word.
`StackChan` / `スタックチャン` is not available as a public microWakeWord model in the checked model repositories. A real StackChan wake word requires a dedicated model training step.

```bash
curl -H "X-StackChan-Token: $TOKEN" \
  "http://$STACKCHAN_IP/wake/status"

curl -X POST "http://$STACKCHAN_IP/wake/start" \
  -H "X-StackChan-Token: $TOKEN"

curl -X POST "http://$STACKCHAN_IP/wake/stop" \
  -H "X-StackChan-Token: $TOKEN"
```

Expected status after wake start:

```json
{
  "ok": true,
  "data": {
    "available": true,
    "modelLoaded": true,
    "running": true,
    "detected": false,
    "engine": "microWakeWord",
    "modelName": "okay_nabu",
    "wakeWord": "Okay Nabu",
    "modelBytes": 60264,
    "lastProbability": 0,
    "averageProbability": 0,
    "debug": {
      "queuedBlocks": 0,
      "processedBlocks": 0,
      "featureFrames": 0,
      "inferenceRuns": 0,
      "lastSamplesConsumed": 0,
      "mic": {
        "rms": 0,
        "peak": 0,
        "maxRms": 0,
        "maxPeak": 0,
        "recordingState": 0
      },
      "features": {
        "rawMin": 0,
        "rawMax": 0,
        "maxRawMax": 0,
        "min": 0,
        "max": 0,
        "maxSeen": 0
      },
      "model": {
        "lastRawOutput": 0,
        "maxRawOutput": 0
      },
      "detected": {
        "maxRawOutput": 0,
        "averageProbability": 0,
        "processedBlocks": 0,
        "inferenceRuns": 0
      }
    },
    "wakeUpload": {
      "inProgress": false,
      "lastHandledDetectedAtMs": 0,
      "lastUploadAtMs": 0,
      "lastHttpStatus": 0,
      "lastError": "",
      "lastSpeechResponse": ""
    },
    "lastError": ""
  }
}
```

`debug` is intentionally kept for wake word tuning. It shows whether audio chunks are queued, features are generated, inference is running, and whether the model output is moving. `debug.model` describes the current wake session; `debug.detected` keeps the last detection snapshot even after wake waiting restarts.
