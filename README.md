# StackChan Lab

**StackChan（M5Stack公式）を外部システムから制御するための、ファームウェアとブリッジの開発リポジトリ。**

顔の表情、LED、サーボ首振り、音声再生、タッチ/ボタン/IMUイベントを HTTP API 経由で操作できます。音声認識（STT）や Wake Word によるハンズフリー操作の基盤も含みます。

## 全体アーキテクチャ

```text
┌─────────────────────────────────────────────────────┐
│                   外部システム                        │
│         （会話エンジン / LLM / ホーム制御）            │
└──────────────────────┬──────────────────────────────┘
                       │ HTTP JSON
                       ▼
┌─────────────────────────────────────────────────────┐
│              stackchan-bridge（Linux/Mac/WSL）        │
│              Node.js / Fastify                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐           │
│  │ HTTP API │  │ 状態管理  │  │ 安全制御  │           │
│  └──────────┘  └──────────┘  └──────────┘           │
│  ┌──────────────────────────────────────┐           │
│  │        Serial  /  Wi-Fi トランスポート │           │
│  └──────────────────────────────────────┘           │
└──────────────────────┬──────────────────────────────┘
                       │ USB Serial または Wi-Fi HTTP
                       ▼
┌─────────────────────────────────────────────────────┐
│          StackChan 本体（M5Stack CoreS3 / ESP32-S3） │
│              C++ / PlatformIO                        │
│  ┌────────┐ ┌────────┐ ┌──────────┐ ┌──────────┐   │
│  │  顔    │ │  LED   │ │ サーボ   │ │  音声    │   │
│  │Avatar  │ │  RGB   │ │ 首振り   │ │  WAV再生 │   │
│  └────────┘ └────────┘ └──────────┘ └──────────┘   │
│  ┌────────┐ ┌────────┐ ┌──────────┐                │
│  │タッチ  │ │ ボタン  │ │   IMU   │                │
│  │イベント│ │ イベント│ │ イベント │                │
│  └────────┘ └────────┘ └──────────┘                │
│  ┌──────────────┐ ┌──────────────┐                  │
│  │ マイク / STT │ │  Wake Word   │                  │
│  │ 録音・転送   │ │  常時待受     │                  │
│  └──────────────┘ └──────────────┘                  │
└─────────────────────────────────────────────────────┘
```

### 責務の分離

| 層 | 責務 | 持たないもの |
|---|---|---|
| **Firmware** | 身体そのもの。表示、LED、首、音声、入力イベント | 会話、人格、LLM、複雑な判断 |
| **Bridge** | 中継層。HTTP→シリアル変換、状態保持、安全制御、イベント中継 | 会話状態、記憶、TTS/STT |
| **外部システム** | 意味処理。会話、人格、記憶、判断 | 身体の低レベル制御 |

## リポジトリ構成

```text
stackchan-lab/
├── firmware/body/      ファームウェア（C++ / PlatformIO）
│   ├── include/        ヘッダ群
│   ├── src/            実装（main.cpp + body/protocol/network/）
│   └── platformio.ini  PlatformIO 設定
│
├── bridge/             ブリッジサーバ（TypeScript / Fastify）
│   ├── src/            実装（bridge/http/serial/transport/stt/）
│   └── package.json    Node.js 設定
│
└── docs/               ドキュメント群
    ├── architecture.md          全体アーキテクチャ
    ├── bridge-api.md            Bridge HTTP API リファレンス
    ├── protocol-serial.md       シリアルプロトコル仕様
    ├── audio-pipeline.md        音声入力・STT・Wake Word パイプライン
    ├── body-features.md         身体表現（プリセット）と入力イベント
    ├── wi-fi-setup.md           Wi-Fi 設定手順
    ├── environment-proxmox.md   Proxmox LXC 環境向け補足
    └── plan/                    開発計画書（非公開・履歴）
```

## ハードウェア要件

| 機材 | 用途 |
|---|---|
| **M5Stack CoreS3** | StackChan 本体（ESP32-S3） |
| **サーボ x2** | 首の水平/垂直駆動 |
| **Linux マシン** | Bridge の実行環境（Node.js）。macOS / WSL も可 |
| USB-C ケーブル | Firmware 書き込み・Serial 通信 |

サーボや筐体は M5Stack 公式 StackChan キットに準拠します。

Bridge 実行ホストの推奨環境:

| 項目 | 最低 | 推奨 |
|---|---|---|
| CPU | 2 コア | 4 コア以上 |
| メモリ | 8 GB | 16 GB |
| OS | Linux / macOS / WSL | Linux (Ubuntu/Debian) |
| Node.js | 22 LTS | 22 LTS |

## クイックスタート

```bash
# 1. Firmware ビルド・書き込み
cd firmware/body
pio run -t upload

# 2. Bridge 起動
cd bridge
cp config.example.yaml config.yaml
npm install
npm run build
npm start

# 3. 動作確認
curl http://127.0.0.1:8787/health
curl -X POST http://127.0.0.1:8787/face \
  -H "Content-Type: application/json" \
  -d '{"expression":"happy"}'
```

手順の詳細は各ディレクトリの README を参照してください。

- [firmware/body/README.md](firmware/body/README.md) — ビルド・書き込み・内部アーキテクチャ
- [bridge/README.md](bridge/README.md) — ブリッジ概要・起動・設定

## ドキュメント一覧

| ドキュメント | 内容 |
|---|---|
| [docs/architecture.md](docs/architecture.md) | 全体アーキテクチャ、データフロー、設計方針 |
| [docs/bridge-api.md](docs/bridge-api.md) | Bridge HTTP API 全エンドポイント + 設定リファレンス |
| [docs/protocol-serial.md](docs/protocol-serial.md) | Firmware シリアルプロトコル仕様（全コマンド・応答） |
| [docs/audio-pipeline.md](docs/audio-pipeline.md) | 音声再生・STT・Wake Word のパイプライン全体 |
| [docs/body-features.md](docs/body-features.md) | 身体プリセットと入力イベント（タッチ/ボタン/IMU） |
| [docs/wi-fi-setup.md](docs/wi-fi-setup.md) | Wi-Fi 接続設定およびデバイス設定の手順 |
| [docs/environment-proxmox.md](docs/environment-proxmox.md) | Proxmox LXC 環境向け USB パススルー設定 |

Firmware / Bridge の個別 README も参照してください。

- [firmware/body/README.md](firmware/body/README.md) — ビルド・書き込み・内部アーキテクチャ
- [bridge/README.md](bridge/README.md) — ブリッジ概要・起動・設定

## トラブルシュート

### シリアルポートの権限エラー（Linux）

```bash
sudo usermod -a -G dialout $USER
# 再ログインが必要
```

### ポートが見つからない

```bash
ls /dev/ttyACM* /dev/ttyUSB*
# macOS の場合: ls /dev/tty.usbmodem*
```

`config.yaml` の `serial.port` を実際のデバイスパスに変更してください。

### Firmware 書き込み失敗: `Wrong boot mode detected`

`platformio.ini` に以下が含まれているか確認:

```ini
board_upload.before_reset = usb_reset
```

### Bridge がシリアルポートを開けない

別プロセス（シリアルモニタ、別の Bridge インスタンス）が使用中でないか確認:

```bash
lsof /dev/ttyACM0   # Linux
```

### `STACKCHAN_NOT_CONNECTED`

- Serial モード: ポートが存在し、Bridge が掴めているか
- Wi-Fi モード: StackChan に `ping` が通るか、IP アドレスが正しいか

### WSL2 から USB シリアルを使う

Windows 側で `usbipd` によるデバイス共有が必要です。

```powershell
# Windows（管理者権限）
usbipd list
usbipd bind -b <busid>
usbipd attach --wsl -b <busid>
```

### Proxmox LXC 環境

Firmware 書き込みに USB パススルー設定が必要です。[docs/environment-proxmox.md](docs/environment-proxmox.md) を参照してください。

## 現在の開発ステータス

| Phase | 内容 | 状態 |
|---|---|---|
| 1-4 | 基本 Firmware + シリアル制御 + Bridge HTTP API | ✅ 完了 |
| 5 | 音声再生 + 身体プリセット | ✅ 完了 |
| 6-7 | 入力イベント + 外部システム接続基盤 | ✅ 完了 |
| 8 | Wi-Fi トランスポート（USB不要の無線制御） | ✅ 完了 |
| 9 | STT パイプライン（外部 stt-adapter 連携） | ✅ 完了 |
| 9.5 | デバイス内蔵 Wake Word（`スタックチャン`専用モデルは未着手） | 🚧 基盤完了 |
