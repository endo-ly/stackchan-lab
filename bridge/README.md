# StackChan Bridge

StackChan Body Firmware を制御するためのローカル HTTP API サーバです。

HTTP JSON リクエストを StackChan デバイスコマンドに変換します。

```text
HTTP クライアント
  → stackchan-bridge
  → USB Serial または Wi-Fi HTTP
  → StackChan Body Firmware
```

会話状態、人格、記憶、TTS は Bridge の責務ではありません。

## 必要条件

- Node.js 22 LTS
- npm
- Serial モード: StackChan との USB Serial 接続
- Wi-Fi モード: StackChan が Wi-Fi に接続済み

## セットアップ

```bash
npm install
npm run build
```

### 設定

```bash
cp config.example.yaml config.yaml
```

`config.yaml` はローカルマシン固有の設定です。Git 管理対象外。

**デフォルト（実機 Serial 接続）**:

```yaml
device:
  transport: "serial"

serial:
  mode: "real"
  port: "/dev/stackchan"
  baud_rate: 115200
```

**Mock モード（実機不要）**:

```yaml
serial:
  mode: "mock"
```

**Wi-Fi モード（無線運用時）**:

事前に [Wi-Fi セットアップ](../docs/wi-fi-setup.md) を完了させてください。

```yaml
device:
  transport: "wifi"

wifi:
  base_url: "http://192.168.0.123"
  token: "<デバイス認証トークン>"
  timeout_ms: 5000
```

**STT 連携（任意）**:

外部の `voice-gateway` を使用する場合:

```yaml
voice_gateway:
  base_url: "http://<voice-gateway>:8012"
  timeout_ms: 120000

agent_runtime:
  endpoint: "http://<egopulse>:10961/api/voice/turn"
  auth_token: "replace-me"
  agent_id: "default"
  surface: "stackchan"
  session_key: "main"
  user_id: "local-speaker"
  timeout_ms: 120000

stt:
  model: "stt-default"

tts:
  model: "aivis-default"
  voice: "your-voice-name"
  response_format: "wav"

wake:
  auto_start: true
  record_duration_ms: 5000

spoken_reply:
  enabled: true
  listen_sources:
    - "stackchan-wake"
  cooldown_ms: 800
  busy_policy: "ignore"

events:
  include_bridge_events: true
```

`spoken_reply.enabled: true` の場合は `agent_runtime.endpoint` と `agent_runtime.auth_token` が必須です。bridge は `AgentClient` で応答テキストを取得し、そのテキストだけを voice-gateway へ渡します。

設定項目の全詳細は [docs/bridge-api.md](../docs/bridge-api.md) の「設定リファレンス」セクションを参照。

### 起動

```bash
# 開発モード
npm run dev

# 本番モード
npm run build
npm start
```

デフォルトで `http://127.0.0.1:8787` にバインドします。

### 動作確認

```bash
npm run smoke
```

```bash
# 手動確認
curl http://127.0.0.1:8787/health
curl http://127.0.0.1:8787/status
curl -X POST http://127.0.0.1:8787/face \
  -H "Content-Type: application/json" \
  -d '{"expression":"happy"}'
```

## 内部アーキテクチャ

```text
main.ts
  │
  ├─ StackChanBridge ──── コマンド送信・レスポンス解析の統括
  │   ├─ presets            身体プリセット（thinking, alert 等）の定義と展開
  │   ├─ safety             サーボ角度の安全クランプ
  │   └─ wav                WAV フォーマット検証
  │
  ├─ HTTP Server ──────── 外部向け REST API
  │   ├─ routes             全エンドポイント定義
  │   └─ BridgeEventStore   イベントキュー管理
  │
  ├─ Transport ────────── デバイス通信の抽象層
  │   ├─ SerialTransport    USB Serial によるコマンド送受信
  │   └─ WifiTransport      Wi-Fi HTTP によるコマンド送受信
  │
  ├─ Serial Protocol ──── USB Serial プロトコル処理
  │   ├─ StackChanClient      クライアント抽象
  │   ├─ StackChanSerialClient  シリアル接続管理
  │   └─ SerialProtocolClient   コマンド送信・レスポンス解析
  │
  └─ CLI ──────────────── セットアップツール
      ├─ wifiSetup          Wi-Fi 設定 CLI
      └─ deviceConfig       デバイス設定転送 CLI
```

### 設計方針

- トランスポート透過: 外部 API は Serial / Wi-Fi どちらでも同一。`config.yaml` の `device.transport` で切り替え
- 安全第一: サーボ角度は Bridge / Firmware の二重クランプ。不正なコマンドは HTTP 400 を返す
- Mock モード: `serial.mode: "mock"` で実機なしでも全機能をテスト可能

### アーキテクチャ上の制約

- TTS は行わない（外部システムの責務）
- STT 推論は行わない（外部 voice-gateway の責務）
- WebSocket / SSE は未実装
- 認証・HTTPS は未実装（LAN 内利用前提）
- 会話状態・長期記憶は持たない

## 操作コマンド一覧

| コマンド | 説明 |
|---|---|
| `npm run dev` | tsx による開発サーバ起動 |
| `npm run build` | TypeScript コンパイル |
| `npm start` | 本番サーバ起動 (`node dist/main.js`) |
| `npm run smoke` | スモークテスト実行 |
| `npm run wifi:setup` | Wi-Fi 設定 CLI（USB Serial 経由） |
| `npm run device:config` | デバイス実行時設定転送 CLI |

## ドキュメント

| 内容 | 参照 |
|---|---|
| API 全エンドポイント・設定リファレンス | [docs/bridge-api.md](../docs/bridge-api.md) |
| 音声パイプライン（WAV 再生 / STT / Wake Word） | [docs/audio-pipeline.md](../docs/audio-pipeline.md) |
| 身体プリセット・イベント | [docs/body-features.md](../docs/body-features.md) |
| Wi-Fi セットアップ | [docs/wi-fi-setup.md](../docs/wi-fi-setup.md) |
| 全体アーキテクチャ | [docs/architecture.md](../docs/architecture.md) |
