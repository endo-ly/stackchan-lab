# アーキテクチャ

## 概要

StackChan Lab は、StackChan の身体を外部システムから HTTP で制御するための **3層アーキテクチャ** を採用しています。

```text
┌──────────────────────────────────────────┐
│              外部システム                  │
│     (LLM / 会話エンジン / ホーム制御)       │
└────────────────────┬─────────────────────┘
                     │ HTTP JSON
┌────────────────────▼─────────────────────┐
│           stackchan-bridge                │
│         Node.js / Fastify                 │
│         (Linux/Mac/WSL 上で動作)          │
│                                           │
│  ┌─────────┐ ┌────────┐ ┌────────────┐   │
│  │HTTP API │ │状態管理 │ │安全クランプ │   │
│  └─────────┘ └────────┘ └────────────┘   │
│  ┌──────────────────────────────────┐    │
│  │     DeviceTransport 抽象層       │    │
│  │  ┌───────────┐ ┌──────────────┐  │    │
│  │  │Serial     │ │WifiTransport │  │    │
│  │  │Transport  │ │  (HTTP)      │  │    │
│  │  └───────────┘ └──────────────┘  │    │
│  └──────────────────────────────────┘    │
└──────────────┬──────────────┬────────────┘
               │ USB Serial   │ Wi-Fi HTTP
┌──────────────▼──────────────▼────────────┐
│       StackChan 本体 (ESP32-S3)           │
│                                       │
│  ┌─────────────────────────────┐      │
│  │     BodyController          │      │
│  │  ┌───────┐┌───┐┌──────┐    │      │
│  │  │Face   ││LED││Motion│    │      │
│  │  │Avatar ││   ││Servo │    │      │
│  │  └───────┘└───┘└──────┘    │      │
│  │  ┌───────┐┌──────────┐     │      │
│  │  │Audio  ││Input     │     │      │
│  │  │WAV再生││Touch/Btn │     │      │
│  │  └───────┘└──────────┘     │      │
│  └─────────────────────────────┘      │
│  ┌──────────────┐ ┌──────────────┐    │
│  │SerialProtocol│ │HttpServer    │    │
│  │(USB Serial)  │ │(Wi-Fi API)   │    │
│  └──────────────┘ └──────────────┘    │
│  ┌──────────────┐ ┌──────────────┐    │
│  │WakeController│ │MicController │    │
│  │(Wake Word)   │ │(→ stt-adapter)│   │
│  └──────────────┘ └──────────────┘    │
└────────────────────────────────────────┘
```

---

## 層ごとの責務

### Firmware 層（StackChan 本体）

| 責務 | 詳細 |
|---|---|
| **身体制御** | 顔（M5Stack-Avatar）、LED RGB、サーボ首振り |
| **音声再生** | WAV 受信・再生・口パク・音量制御 |
| **入力検出** | タッチ、ボタン、IMU のイベント検出とキュー管理 |
| **通信** | USB Serial（テキストプロトコル）と Wi-Fi HTTP の二系統 |
| **マイク** | 16kHz モノラル録音、STT サービスへの WAV アップロード |
| **Wake Word** | microWakeWord 方式の常時待受・検出（基盤完了） |

**持たないもの**: 会話状態、人格、LLM、TTS/STT 推論

### Bridge 層（Bridge 実行ホスト）

| 責務 | 詳細 |
|---|---|
| **HTTP API** | Fastify による REST API（`/face`, `/led`, `/pose`, `/move`, `/play-wav` 他） |
| **プロトコル変換** | HTTP JSON リクエストをシリアルコマンドに変換・送信 |
| **トランスポート抽象** | `DeviceTransport` インターフェースで Serial / Wi-Fi を切り替え可能 |
| **状態管理** | 最終コマンド・最終応答のキャッシュ、接続状態の監視 |
| **安全制御** | サーボ角度のクランプ（設定可能な安全範囲）、不正値のバリデーション |
| **イベント中継** | Firmware イベントの取得・キャッシュ、STT イベントの受信・統合 |
| **音声中継** | WAV ファイルの Firmware への転送（multipart → バイナリ） |

**持たないもの**: TTS、会話状態、長期記憶、WebSocket（将来的に追加可能性あり）

### 外部システム層

| 責務 | 詳細 |
|---|---|
| **会話制御** | LLM との対話、人格管理、文脈保持 |
| **TTS** | テキスト→音声変換。生成した WAV を `/play-wav` で再生 |
| **STT** | 音声→テキスト変換。外部 `stt-adapter` として独立サービス化 |
| **判断** | ユーザ意図の解釈、身体表現プリセットの選択 |

---

## データフロー

### 制御フロー（外部→StackChan）

```text
外部システム
  │ POST /face {"expression":"happy"}
  ▼
Bridge HTTP API
  │ バリデーション（表情が許可値か）
  │ トランスポート選択（serial / wifi）
  ▼
SerialTransport or WifiTransport
  │ "FACE:happy\n" を送信
  ▼
StackChan Firmware
  │ CommandParser → CommandHandler → BodyController.setExpression()
  ▼
FaceController（M5Stack-Avatar）
  │ 表情を happy に変更
  ▼
OK FACE happy (応答)
  │ Bridge がレスポンスをパースして HTTP 200 を返す
  ▼
外部システムへ応答
```

### イベントフロー（StackChan→外部）

```text
StackChan Firmware
  │ タッチ検出 → InputController が InputEvent をキューに追加
  ▼
外部システム
  │ GET /events をポーリング（または後段で WebSocket 対応）
  ▼
Bridge
  │ "EVENTS\n" を送信
  ▼
Firmware
  │ キュー内容をレスポンスとして返す
  ▼
Bridge
  │ HTTP JSON にマッピングして外部システムへ返す
```

### 音声パイプライン（Wake Word → STT → 応答）

```text
StackChan マイク
  │ 16kHz 継続取得
  ▼
WakeController（Firmware）
  │ 10ms block 分割 → 特徴量生成 → microWakeWord TFLite 推論
  │ wake word 検出 → 3秒録音 → stt-adapter へ WAV アップロード
  ▼
stt-adapter（外部 Python サービス）
  │ 音声認識（ReazonSpeech K2 v2 等）
  │ 認識結果を Bridge の POST /stt/events へ送信
  ▼
Bridge
  │ STT イベントをイベントキューに保存
  ▼
外部システム
  │ GET /stt/latest でテキストを取得 → LLM 処理 → 応答生成
```

---

## トランスポート設計

Bridge は `DeviceTransport` インターフェースにより、物理的な接続方式を抽象化しています。

### Serial モード

```text
Bridge → USB Serial (/dev/stackchan) → StackChan
```

| 項目 | 値 |
|---|---|
| ボーレート | 115200 |
| プロトコル | テキスト行ベース |
| 用途 | 開発時、Firmware 書き込みと同じポート |

### Wi-Fi モード

```text
Bridge → HTTP over LAN → StackChan (Wi-Fi AP に接続)
```

| 項目 | 値 |
|---|---|
| 認証 | `X-StackChan-Token` ヘッダ（任意、未設定時は開発モード） |
| 用途 | 設置後の無線運用、USB ケーブル不要 |
| セットアップ | USB Serial 経由で一度だけ Wi-Fi 設定を保存 |

詳細は [wi-fi-setup.md](wi-fi-setup.md) を参照。

---

## 設計方針

### Firmware 側

1. **身体命令だけを受け付ける**
   - `FACE:happy` のような低レベル命令のみ。`THINK_ABOUT_USER` のような意味命令は持たない
   - 意味解釈は必ず外部システムが行う

2. **Controller 分割**
   - `BodyController` が全サブコントローラを統括
   - `FaceController` が M5Stack-Avatar への依存を内部に閉じ込める
   - `main.cpp` は `begin()` / `update()` の呼び出しだけに徹する

3. **デュアル通信路**
   - USB Serial と Wi-Fi HTTP の両方を常時有効化
   - Wi-Fi 設定がない / 接続失敗時も Serial は動作する

### Bridge 側

1. **トランスポート透過**
   - 外部 API は Serial/Wi-Fi どちらでも同一
   - `config.yaml` の `device.transport` で切り替え

2. **安全第一**
   - サーボ角度は Bridge / Firmware の二重クランプ
   - 不正なコマンドは HTTP 400 を返す

3. **Mock モード**
   - Firmware 実機がなくても Bridge の全機能をテスト可能
   - `serial.mode: "mock"` で有効

---

## 主要コンポーネント一覧

### Firmware（C++）

| コンポーネント | パス | 役割 |
|---|---|---|
| `BodyController` | `body/BodyController` | 全身体制御の統括 |
| `FaceController` | `body/FaceController` | M5Stack-Avatar による顔描画・表情・瞬き |
| `LedController` | `body/LedController` | RGB LED のムード制御 |
| `MotionController` | `body/MotionController` | サーボ駆動・安全クランプ |
| `AudioController` | `body/AudioController` | WAV 受信・再生・音量 |
| `InputController` | `body/InputController` | タッチ/ボタン/IMU のイベント検出 |
| `MicController` | `body/MicController` | マイク録音・WAV 生成・STT アップロード |
| `WakeController` | `body/WakeController` | Wake Word 常時待受・推論 |
| `SerialProtocol` | `protocol/SerialProtocol` | USB Serial コマンド解析 |
| `CommandHandler` | `protocol/CommandHandler` | コマンド→身体制御の振り分け |
| `HttpServerController` | `network/HttpServerController` | Wi-Fi HTTP API |
| `NetworkController` | `network/NetworkController` | Wi-Fi 接続管理 |
| `DeviceAuth` | `network/DeviceAuth` | トークン認証 |

### Bridge（TypeScript）

| コンポーネント | パス | 役割 |
|---|---|---|
| `StackChanBridge` | `bridge/StackChanBridge` | コマンド送信・レスポンス解析の統括 |
| `createServer` | `http/createServer` | Fastify HTTP サーバ構築 |
| `routes` | `http/routes` | 全エンドポイント定義 |
| `SerialTransport` | `transport/SerialTransport` | USB Serial トランスポート実装 |
| `WifiTransport` | `transport/WifiTransport` | Wi-Fi HTTP トランスポート実装 |
| `BridgeEventStore` | `events/BridgeEventStore` | ブリッジ側イベントキュー |
| `presets` | `bridge/presets` | 身体プリセット定義 |
| `safety` | `bridge/safety` | サーボ安全クランプ |
| `wav` | `bridge/wav` | WAV ファイル検証 |
| `loadConfig` | `config/loadConfig` | YAML 設定読み込み |
| `MockStackChanClient` | `mock/MockStackChanClient` | 実機不要のテスト用モック |
| `StackChanClient` | `serial/StackChanClient` | クライアント抽象インターフェース |
