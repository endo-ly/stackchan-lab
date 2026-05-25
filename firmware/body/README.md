# StackChan Body Firmware

M5Stack 公式 StackChan（CoreS3 / ESP32-S3）向けの PlatformIO ファームウェアです。

USB Serial と Wi-Fi HTTP の二系統で身体制御コマンドを受け付け、顔・LED・サーボ・音声・入力イベントを管理します。

## 必要条件

- PlatformIO
- M5Stack CoreS3（StackChan 本体）
- USB 接続
- 2.4GHz Wi-Fi AP（Wi-Fi モード使用時）

依存ライブラリは `platformio.ini` で自動解決されます。

| ライブラリ | 用途 |
|---|---|
| `StackChan-BSP` | M5Stack 公式ボードサポートパッケージ |
| `M5Stack-Avatar` | 顔描画・表情・瞬き |
| `ArduinoJson` | JSON パース |
| `esp-micro-speech-features` | Wake Word の特徴量生成 |
| `MicroTFLite` | Wake Word の TFLite 推論 |

## セットアップ

### PlatformIO のインストール

```bash
# pip 経由（推奨）
pip install platformio

# または VS Code 拡張として
# code --install-extension platformio.platformio-ide
```

### ビルド

```bash
cd firmware/body
pio run
```

### 書き込み

```bash
pio run -t upload
# 明示する場合:
pio run -t upload --upload-port /dev/stackchan
```

### シリアルモニタ

```bash
pio device monitor -p /dev/stackchan -b 115200
```

期待される起動ログ:

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

### 開発環境（Proxmox LXC）

Proxmox LXC 内から書き込む場合の USB パススルー設定は [docs/environment-proxmox.md](../docs/environment-proxmox.md) を参照してください。

## 内部アーキテクチャ

```text
main.cpp
  │
  ├─ BodyController ──── 全身体制御の統括
  │   ├─ AudioController    WAV 再生・音量
  │   ├─ DisplayController  画面管理
  │   ├─ FaceController     M5Stack-Avatar（表情・瞬き・口パク）
  │   ├─ InputController    タッチ/ボタン/IMU イベント検出
  │   ├─ LedController      RGB LED ムード制御
  │   ├─ MicController      マイク録音・STT アップロード
  │   └─ MotionController   サーボ駆動・安全クランプ
  │
  ├─ SerialProtocol ─── USB Serial コマンド受信
  │   ├─ CommandParser      コマンド解析
  │   └─ CommandHandler     BodyController への振り分け
  │
  ├─ NetworkController ── Wi-Fi 接続管理
  │   ├─ WiFiManager        設定保存・接続
  │   ├─ DeviceAuth         トークン認証
  │   └─ HttpServerController  HTTP API 提供
  │
  └─ WakeController ──── Wake Word 常時待受
      ├─ マイク継続取得
      ├─ 特徴量生成 (esp-micro-speech-features)
      ├─ TFLite 推論 (MicroTFLite)
      └─ 検出後 3 秒録音・STT アップロード
```

### 設計方針

- `main.cpp` は `begin()` / `update()` の呼び出しだけに徹する
- `FaceController` が M5Stack-Avatar への依存を内部に閉じ込める
- `CommandHandler` は顔描画の詳細を知らず、`BodyController.setExpression()` だけを呼ぶ
- `MotionController` は安全範囲外の値をクランプし、警告をログ出力する
- シリアル通信と Wi-Fi HTTP の両方を常時有効化（Wi-Fi 設定なしでも Serial は動作）
- `[` で始まる行はデバッグログ。コマンド応答と区別すること

### 安全範囲（deci-degree）

| 軸 | 最小 | 最大 |
|---|---|---|
| X | -450 | 450 |
| Y | 0 | 450 |

## 操作コマンド一覧

シリアルモニタで直接入力して動作確認できます。完全なプロトコル仕様は [docs/protocol-serial.md](../docs/protocol-serial.md) を参照。

```text
PING
VERSION
STATUS
FACE:happy
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

基本レスポンス:

```text
OK PONG
OK VERSION firmware=0.8.0 protocol=0.1.0 board=stackchan-cores3
OK STATUS mode=Ready expression=Neutral mood=Calm pose=Neutral x=0 y=0 ...
OK FACE happy
ERR INVALID_ARGUMENT expression=thinking
```

## ドキュメント

| 内容 | 参照 |
|---|---|
| シリアルプロトコル完全仕様 | [docs/protocol-serial.md](../docs/protocol-serial.md) |
| Wi-Fi セットアップ手順 | [docs/wi-fi-setup.md](../docs/wi-fi-setup.md) |
| 音声パイプライン（再生/STT/Wake Word） | [docs/audio-pipeline.md](../docs/audio-pipeline.md) |
| 身体プリセット・イベント | [docs/body-features.md](../docs/body-features.md) |
| 全体アーキテクチャ | [docs/architecture.md](../docs/architecture.md) |
