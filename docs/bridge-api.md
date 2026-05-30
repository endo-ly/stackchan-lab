# Bridge API リファレンス

StackChan の身体（表情・サーボ・LED・音声・イベント）を操作する HTTP API の完全な仕様です。

- **Base URL**: `http://127.0.0.1:8787`
- **Content-Type**: `application/json`（`/play-wav` のみ `multipart/form-data`）

Bridge の概要・起動方法については [bridge/README.md](../bridge/README.md) を参照してください。

---

## レスポンス形式

### 成功

```json
{ "ok": true, "data": { ... } }
```

### エラー

```json
{
  "ok": false,
  "error": {
    "code": "INVALID_ARGUMENT",
    "message": "Unsupported expression: thinking"
  }
}
```

---

## エンドポイント一覧

### システム

| メソッド | パス | 説明 |
|---|---|---|
| `GET` | `/health` | プロセス死活確認 |
| `GET` | `/version` | Bridge・Firmware バージョン |
| `GET` | `/capabilities` | サポート値一覧 |

### 身体制御

| メソッド | パス | 説明 |
|---|---|---|
| `GET` | `/status` | 現在の身体状態 |
| `POST` | `/face` | 表情変更 |
| `POST` | `/led` | LED ムード変更 |
| `POST` | `/pose` | サーボをプリ定義ポーズへ移動 |
| `POST` | `/move` | サーボを X/Y 座標で直接移動 |
| `POST` | `/reset` | 身体を安全なニュートラル状態に戻す |

### 音声

| メソッド | パス | 説明 |
|---|---|---|
| `GET` | `/audio/status` | 音声再生状態 |
| `POST` | `/audio/volume` | 音量設定 (0-255) |
| `POST` | `/audio/stop` | 再生停止 |
| `POST` | `/play-wav` | WAV 送信・再生 |

### イベント

| メソッド | パス | 説明 |
|---|---|---|
| `GET` | `/events` | イベントキュー取得（読み取りのみ） |
| `GET` | `/events/latest` | 最新イベント 1 件 |
| `POST` | `/events/clear` | イベントキューをクリア |

### プリセット

| メソッド | パス | 説明 |
|---|---|---|
| `GET` | `/presets` | 利用可能なプリセット一覧 |
| `POST` | `/preset` | プリセット適用 |

### STT（音声認識）

| メソッド | パス | 説明 |
|---|---|---|
| `GET` | `/stt/latest` | 最新の STT 結果 |
| `POST` | `/stt/events` | stt-adapter からの転写完了イベント受信 |

---

## エンドポイント詳細

### GET /health

プロセス死活確認。

```bash
curl http://127.0.0.1:8787/health
```

**Response 200**

```json
{ "ok": true, "data": { "service": "stackchan-bridge", "status": "ok" } }
```

---

### GET /version

Bridge およびデバイスファームウェアのバージョン。

```bash
curl http://127.0.0.1:8787/version
```

**Response 200**

```json
{
  "ok": true,
  "data": {
    "bridge": { "version": "0.8.0" },
    "device": {
      "connected": true,
      "firmware": "0.8.0",
      "protocol": "0.1.0",
      "board": "stackchan-cores3"
    }
  }
}
```

デバイス未接続時: `device.connected: false`

---

### GET /capabilities

サポートされている値の一覧。

```bash
curl http://127.0.0.1:8787/capabilities
```

**Response 200**

```json
{
  "ok": true,
  "data": {
    "expressions": ["neutral", "happy", "sad", "angry", "sleepy", "doubt"],
    "moods": ["calm", "active", "speaking", "warning", "off"],
    "poses": ["neutral", "look_left", "look_right", "look_up", "look_down"],
    "commands": ["PING", "VERSION", "HELP", "STATUS", "FACE", "LED", "POSE", "MOVE", "RESET", "AUDIO", "EVENTS"]
  }
}
```

---

### GET /status

現在の身体状態。Bridge の最終キャッシュを返します（都度 Firmware に問い合わせません）。

```bash
curl http://127.0.0.1:8787/status
```

**Response 200**

```json
{
  "ok": true,
  "data": {
    "connected": true,
    "mode": "Ready",
    "expression": "Neutral",
    "mood": "Calm",
    "pose": "Neutral",
    "x": 0,
    "y": 0,
    "gazeX": 0,
    "gazeY": 0,
    "speaking": false,
    "blinking": false,
    "eventCount": 0,
    "latestEvent": "none",
    "latestTarget": "none",
    "latestValue": "none",
    "touchActive": false,
    "buttonActive": false,
    "imuMoving": false,
    "lastCommandAt": "2025-01-15T10:30:00.000Z",
    "lastResponseAt": "2025-01-15T10:30:00.123Z"
  }
}
```

**フィールド**

| Field | Type | 説明 |
|---|---|---|
| `connected` | boolean | デバイス接続状態 |
| `mode` | string | 動作モード（`Ready` など） |
| `expression` | string | 現在の表情 |
| `mood` | string | 現在の LED ムード |
| `pose` | string | 現在のポーズ |
| `x` | number | サーボ X 軸（deci-degree） |
| `y` | number | サーボ Y 軸（deci-degree） |
| `gazeX` | number | 視線 X |
| `gazeY` | number | 視線 Y |
| `speaking` | boolean | 発話中か |
| `blinking` | boolean | 瞬き中か |
| `eventCount` | number | イベントキュー内のイベント数 |
| `latestEvent` | string | 最新イベントの種類 |
| `latestTarget` | string | 最新イベントの発生源 |
| `latestValue` | string | 最新イベントの値 |
| `touchActive` | boolean | タッチ中か |
| `buttonActive` | boolean | ボタン押下中か |
| `imuMoving` | boolean | IMU が動きを検出したか |
| `lastCommandAt` | string | 最終コマンド送信時刻（ISO 8601） |
| `lastResponseAt` | string | 最終レスポンス受信時刻（ISO 8601） |

---

### POST /face

表情を変更する。

```bash
curl -X POST http://127.0.0.1:8787/face \
  -H "Content-Type: application/json" \
  -d '{"expression":"doubt"}'
```

**Request**

| Field | Type | Required | Values |
|---|---|---|---|
| expression | string | Yes | `neutral`, `happy`, `sad`, `angry`, `sleepy`, `doubt` |

**Response 200**

```json
{ "ok": true, "data": { "expression": "doubt" } }
```

> **備考**: `thinking` と `alert` は `FACE` の値ではなく、`POST /preset` のプリセット名です。これらを `/face` に指定すると `INVALID_ARGUMENT` になります。

---

### POST /led

LED ムードを変更する。

```bash
curl -X POST http://127.0.0.1:8787/led \
  -H "Content-Type: application/json" \
  -d '{"mood":"calm"}'
```

**Request**

| Field | Type | Required | Values |
|---|---|---|---|
| mood | string | Yes | `calm`, `active`, `speaking`, `warning`, `off` |

**Response 200**

```json
{ "ok": true, "data": { "mood": "calm" } }
```

---

### POST /pose

プリ定義ポーズに移動する。

```bash
curl -X POST http://127.0.0.1:8787/pose \
  -H "Content-Type: application/json" \
  -d '{"pose":"look_left"}'
```

**Request**

| Field | Type | Required | Values |
|---|---|---|---|
| pose | string | Yes | `neutral`, `look_left`, `look_right`, `look_up`, `look_down` |

**Response 200**

```json
{ "ok": true, "data": { "pose": "look_left" } }
```

---

### POST /move

サーボを X/Y で直接移動する。単位は **deci-degree**（`10 = 1度`）。Bridge と Firmware の二重で安全範囲クランプがかかります。

```bash
curl -X POST http://127.0.0.1:8787/move \
  -H "Content-Type: application/json" \
  -d '{"x":0,"y":450}'
```

**Request**

| Field | Type | Required | 説明 |
|---|---|---|---|
| x | number | Yes | 水平方向（Bridge 側デフォルト: -300 〜 300） |
| y | number | Yes | 垂直方向（Bridge 側デフォルト: 0 〜 450） |

**Response 200**（クランプなし）

```json
{ "ok": true, "data": { "x": 0, "y": 450, "clamped": false } }
```

**Response 200**（クランプあり）

```json
{
  "ok": true,
  "data": {
    "x": 300,
    "y": 450,
    "clamped": true,
    "requested": { "x": 999, "y": 450 }
  }
}
```

---

### POST /reset

身体を安全なニュートラル状態に戻す（デバイス再起動ではない）。

```bash
curl -X POST http://127.0.0.1:8787/reset \
  -H "Content-Type: application/json" \
  -d '{}'
```

**Response 200**

```json
{ "ok": true, "data": { "reset": true } }
```

---

### GET /audio/status

音声再生状態を取得する。

```bash
curl http://127.0.0.1:8787/audio/status
```

**Response 200**

```json
{
  "ok": true,
  "data": {
    "state": "Idle",
    "playing": false,
    "volume": 180,
    "size": 0,
    "received": 0
  }
}
```

| Field | Type | 説明 |
|---|---|---|
| state | string | `Idle`, `Receiving`, `Playing`, `Finished`, `Error` |
| playing | boolean | 再生中フラグ |
| volume | number | 音量（0-255） |
| size | number | 受信予定バイト数 |
| received | number | 受信済みバイト数 |

---

### POST /audio/volume

音量を設定する。

```bash
curl -X POST http://127.0.0.1:8787/audio/volume \
  -H "Content-Type: application/json" \
  -d '{"volume":180}'
```

**Request**

| Field | Type | Required | 説明 |
|---|---|---|---|
| volume | number | Yes | 0 〜 255 の整数 |

**Response 200**

```json
{ "ok": true, "data": { "volume": 180 } }
```

---

### POST /audio/stop

音声再生を停止する。

```bash
curl -X POST http://127.0.0.1:8787/audio/stop
```

**Response 200**

```json
{ "ok": true, "data": { "stopped": true } }
```

---

### POST /play-wav

WAV ファイルを送信して再生する。

```bash
curl -X POST http://127.0.0.1:8787/play-wav \
  -F "file=@test-assets/sample.wav"
```

**Request**: `multipart/form-data`

| Field | Type | Required | 説明 |
|---|---|---|---|
| file | file | Yes | WAV ファイル（最大 1MB） |

**対応 WAV 形式**

- PCM / mono / 16-bit
- 任意の有効サンプルレート（16000Hz、24000Hz、44100Hz 等）
- 最大 1MB（Firmware 側でチェック）

**Response 200**

```json
{ "ok": true, "data": { "playing": true, "size": 245760 } }
```

---

### GET /events

入力イベントキューを取得する。読み取りだけではキューはクリアされません。

イベントの種類と仕組みの詳細は [body-features.md](body-features.md) を参照。

```bash
curl http://127.0.0.1:8787/events
```

**Response 200**

```json
{
  "ok": true,
  "data": {
    "count": 2,
    "events": [
      {
        "id": 1,
        "type": "touch",
        "target": "screen",
        "value": "tap",
        "timestamp": 12345678,
        "x": 123,
        "y": 87
      },
      {
        "id": 2,
        "type": "imu",
        "target": "device",
        "value": "shake",
        "timestamp": 12345900
      }
    ]
  }
}
```

**イベントフィールド**

| Field | Type | 説明 |
|---|---|---|
| id | number | ブートからの通し番号 |
| type | string | `touch`, `button`, `imu` |
| target | string | `screen`, `button_a`, `button_b`, `button_c`, `device` |
| value | string | `tap`, `pressed`, `moved`, `shake` |
| timestamp | number | ミリ秒タイムスタンプ |
| x | number | タッチ X 座標（`type: "touch"` のみ） |
| y | number | タッチ Y 座標（`type: "touch"` のみ） |

---

### GET /events/latest

最新のイベントを 1 件取得する。

```bash
curl http://127.0.0.1:8787/events/latest
```

**Response 200**（イベントあり）

```json
{
  "ok": true,
  "data": {
    "event": {
      "id": 2,
      "type": "imu",
      "target": "device",
      "value": "shake",
      "timestamp": 12345900
    }
  }
}
```

**Response 200**（イベントなし）

```json
{ "ok": true, "data": { "event": null } }
```

---

### POST /events/clear

イベントキューをクリアする。Firmware 側と Bridge 側の両方をクリアします。

```bash
curl -X POST http://127.0.0.1:8787/events/clear
```

**Response 200**

```json
{ "ok": true, "data": { "cleared": true } }
```

---

### GET /presets

利用可能な body preset 一覧。

```bash
curl http://127.0.0.1:8787/presets
```

**Response 200**

```json
{
  "ok": true,
  "data": {
    "presets": ["idle", "thinking", "alert", "listening", "speaking", "happy", "sleepy", "error"]
  }
}
```

各プリセットの意味と用途は [body-features.md](body-features.md) を参照。

---

### POST /preset

body preset を適用する。

```bash
curl -X POST http://127.0.0.1:8787/preset \
  -H "Content-Type: application/json" \
  -d '{"name":"thinking"}'
```

**Request**

| Field | Type | Required | Values |
|---|---|---|---|
| name | string | Yes | `idle`, `thinking`, `alert`, `listening`, `speaking`, `happy`, `sleepy`, `error` |

**Response 200**

```json
{
  "ok": true,
  "data": {
    "preset": "thinking",
    "applied": {
      "expression": "doubt",
      "mood": "calm",
      "pose": "neutral"
    }
  }
}
```

プリセットは Bridge 側で FACE / LED / POSE に展開されてから Firmware へ送られます。Firmware 側にプリセット解決ロジックはありません。

---

### GET /stt/latest

最新の STT（音声認識）結果を取得する。存在しない場合は `null`。

```bash
curl http://127.0.0.1:8787/stt/latest
```

**Response 200**（結果あり）

```json
{
  "ok": true,
  "data": {
    "text": "こんにちは",
    "language": "ja",
    "durationSec": 3.2,
    "processingMs": 820,
    "provider": "reazonspeech_k2",
    "model": "reazon-research/reazonspeech-k2-v2",
    "receivedAt": "2025-01-15T10:30:00.000Z"
  }
}
```

**Response 200**（結果なし）

```json
{ "ok": true, "data": null }
```

---

### POST /stt/events

外部の `stt-adapter` から転写完了イベントを受け取るエンドポイント。Bridge はこの結果を `/stt/latest` と `/events` に反映します。

```bash
curl -X POST http://127.0.0.1:8787/stt/events \
  -H "Content-Type: application/json" \
  -d '{
    "source": "stackchan",
    "text": "こんにちは",
    "language": "ja",
    "durationSec": 3.2,
    "processingMs": 820,
    "provider": "reazonspeech_k2",
    "model": "reazon-research/reazonspeech-k2-v2",
    "audio": {
      "sampleRate": 16000,
      "channels": 1,
      "format": "wav"
    }
  }'
```

**Request**

| Field | Type | Required | 説明 |
|---|---|---|---|
| source | string | Yes | 音声ソース（`"stackchan"` など） |
| text | string | Yes | 認識テキスト |
| language | string | No | 言語コード |
| durationSec | number | No | 音声の秒数 |
| processingMs | number | No | STT 処理時間（ミリ秒） |
| provider | string | No | STT プロバイダ名 |
| model | string | No | 使用モデル名 |
| audio | object | No | 音声メタ情報 |

**Response 200**

```json
{
  "ok": true,
  "data": {
    "stored": true,
    "processingMs": 820
  }
}
```

---

## エラーコード一覧

| Error Code | HTTP | 説明 |
|---|---|---|
| `INVALID_REQUEST` | 400 | リクエスト形式が不正 |
| `INVALID_ARGUMENT` | 400 | 引数の値が許可範囲外 |
| `CONFIG_ERROR` | 400 | 設定値が不正 |
| `PROTOCOL_PARSE_ERROR` | 400 | プロトコル応答の解析失敗 |
| `EVENT_PARSE_ERROR` | 400 | イベントデータの解析失敗 |
| `AUDIO_TOO_LARGE` | 400 | WAV ファイルが上限超過 |
| `AUDIO_INVALID_FORMAT` | 400 | WAV フォーマットがサポート外 |
| `UNSUPPORTED_PRESET` | 400 | 不明な preset 名 |
| `DEVICE_UNAUTHORIZED` | 502 | デバイス認証トークンが不正 |
| `STACKCHAN_NOT_CONNECTED` | 503 | デバイス未接続 |
| `DEVICE_UNREACHABLE` | 503 | デバイスに到達不能 |
| `AUDIO_BUSY` | 503 | 音声再生中で新規受け付け不可 |
| `STACKCHAN_TIMEOUT` | 504 | デバイス応答タイムアウト |
| `DEVICE_TIMEOUT` | 504 | デバイス応答タイムアウト |
| `EVENTS_TIMEOUT` | 504 | イベント取得タイムアウト |
| `AUDIO_RECEIVE_TIMEOUT` | 504 | WAV 転送タイムアウト |
| `STACKCHAN_ERROR` | 500 | デバイスがエラー応答 |
| `TRANSPORT_ERROR` | 500 | トランスポート層エラー |
| `TRANSPORT_NOT_CONFIGURED` | 500 | 選択されたトランスポートが未設定 |
| `AUDIO_TRANSFER_FAILED` | 500 | WAV 転送に失敗 |
| `PRESET_APPLY_FAILED` | 500 | preset 適用中にエラー |
| `EVENT_QUEUE_ERROR` | 500 | イベントキューの内部エラー |
| `UNSUPPORTED_INPUT` | 500 | サポートされない入力 |
| `INTERNAL_ERROR` | 500 | 予期しない内部エラー |

---

## 設定リファレンス

`config.yaml` の全設定項目。

```yaml
# === サーバ設定 ===
server:
  host: "127.0.0.1"       # バインドアドレス
  port: 8787              # リスンポート

# === デバイス接続 ===
device:
  transport: "serial"     # "serial" | "wifi"

# === Serial モード ===
serial:
  mode: "real"            # "real" | "mock"
  port: "/dev/stackchan"  # シリアルポートパス
  baud_rate: 115200       # ボーレート
  timeout_ms: 3000        # コマンド応答タイムアウト（ミリ秒）
  reconnect:
    enabled: true         # 自動再接続
    interval_ms: 2000     # 再接続間隔（ミリ秒）

# === Wi-Fi モード ===
wifi:
  base_url: "http://192.168.0.123"  # StackChan の IP
  token: ""                         # デバイス認証トークン（Wi-Fi セットアップ時に設定）
  timeout_ms: 5000                  # HTTP リクエストタイムアウト（ミリ秒）
  health_check_interval_ms: 5000    # ヘルスチェック間隔（ミリ秒）

# === STT 連携（任意） ===
stt:
  transcribe_url: "http://<ip>:8790/transcribe"  # stt-adapter のエンドポイント

# === イベント ===
events:
  include_bridge_events: true  # Bridge 側イベントを /events に含めるか

# === 安全設定 ===
safety:
  min_x: -300   # サーボ X 軸 最小（deci-degree）
  max_x: 300    # サーボ X 軸 最大
  min_y: 0      # サーボ Y 軸 最小
  max_y: 450    # サーボ Y 軸 最大

# === ログ ===
logging:
  level: "info"  # "debug" | "info" | "warn" | "error"
```

### Serial モード vs Mock モード

| 項目 | `mode: "real"` | `mode: "mock"` |
|---|---|---|
| 実機要否 | 必要 | 不要 |
| シリアルポート | 実デバイス (`/dev/stackchan`) | 仮想 |
| 用途 | 本番運用・実機テスト | Bridge 単体開発・CI |

### Wi-Fi モードの注意点

- Wi-Fi 設定は USB Serial 経由で StackChan 本体に保存します。設定手順は [wi-fi-setup.md](wi-fi-setup.md) を参照。
- `wifi.token` が空文字列の場合、StackChan 本体の HTTP API は **認証なし**（開発モード）で動作します。
- 本番運用時は `openssl rand -hex 32` でトークンを生成し、Bridge と StackChan の両方に同じ値を設定してください。
