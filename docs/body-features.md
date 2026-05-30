# 身体機能

StackChan の身体表現（プリセット）と入力検出（イベント）の仕様です。

---

## 1. 身体プリセット

### 設計思想

プリセットは **身体表現のショートカット** です。外部システムが `thinking` という 1 つの名前を指定するだけで、表情・LED・ポーズの適切な組み合わせが適用されます。

```text
外部システム
  │ POST /preset {"name":"thinking"}
  ▼
Bridge  プリセット展開
  │ thinking = FACE:doubt + LED:calm + POSE:neutral
  │ 3 つの低レベルコマンドに分解して送信
  ▼
Firmware  身体制御
```

**設計上のポイント**:

- プリセット解決ロジックは **Bridge 側** にあります。Firmware はプリセットを知りません。
- Firmware が受け取るのは `FACE:doubt` のような低レベル命令のみです。
- プリセットの追加・変更は Bridge のコード（`src/bridge/presets.ts`）の変更だけで済み、Firmware の再書き込みは不要です。

### プリセット一覧

| プリセット | 表情 | LED | ポーズ | 用途 |
|---|---|---|---|---|
| `idle` | neutral | calm | neutral | 待機中・デフォルト |
| `thinking` | doubt | calm | neutral | LLM の思考中 |
| `alert` | neutral | warning | neutral | 注意喚起・Wake Word 検出時 |
| `listening` | neutral | active | neutral | ユーザ発話の聞き取り中 |
| `speaking` | neutral | speaking | neutral | TTS 音声再生中 |
| `happy` | happy | active | neutral | 肯定応答・喜び |
| `sleepy` | sleepy | calm | neutral | スリープモード |
| `error` | doubt | warning | neutral | エラー発生時 |

### Bridge API

```bash
# プリセット一覧
curl http://127.0.0.1:8787/presets

# プリセット適用
curl -X POST http://127.0.0.1:8787/preset \
  -H "Content-Type: application/json" \
  -d '{"name":"thinking"}'
```

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

### プリセットの拡張

`bridge/src/bridge/presets.ts` の `bodyPresets` オブジェクトにエントリを追加します。

```typescript
export const bodyPresets: Record<PresetName, BodyPreset> = {
  // ... 既存 ...
  excited: { expression: "happy", mood: "speaking", pose: "look_up" },
};
```

> **注意**: プリセット名は `bridge/src/types/body.ts` の `PresetName` 型にも追加する必要があります。

---

## 2. 表情・LED・ポーズの単独操作

プリセットを使わず、表情・LED・ポーズを個別に操作することもできます。

### 表情（FACE）

```bash
curl -X POST http://127.0.0.1:8787/face \
  -H "Content-Type: application/json" \
  -d '{"expression":"happy"}'
```

| 値 | 説明 |
|---|---|
| `neutral` | 通常 |
| `happy` | 笑顔 |
| `sad` | 悲しい |
| `angry` | 怒り |
| `sleepy` | 眠そう |
| `doubt` | 困惑 |

`thinking` と `alert` は `FACE` の値ではありません。これらはプリセット名です。誤って `/face` に指定すると `INVALID_ARGUMENT` エラーになります。

### LED

```bash
curl -X POST http://127.0.0.1:8787/led \
  -H "Content-Type: application/json" \
  -d '{"mood":"active"}'
```

| 値 | 説明 |
|---|---|
| `calm` | 穏やか（低輝度・単色） |
| `active` | 活動中（中輝度） |
| `speaking` | 発話中（口パク連動） |
| `warning` | 警告（赤系・点滅） |
| `off` | 消灯 |

### ポーズ（POSE）

```bash
curl -X POST http://127.0.0.1:8787/pose \
  -H "Content-Type: application/json" \
  -d '{"pose":"look_left"}'
```

| 値 | 説明 |
|---|---|
| `neutral` | 正面 |
| `look_left` | 左向き |
| `look_right` | 右向き |
| `look_up` | 上向き |
| `look_down` | 下向き |

### 直接移動（MOVE）

ポーズより細かい制御が必要な場合は、サーボを X/Y 座標で直接指定します。

```bash
curl -X POST http://127.0.0.1:8787/move \
  -H "Content-Type: application/json" \
  -d '{"x":100,"y":300}'
```

単位は **deci-degree**（`10 = 1度`）。Bridge 側と Firmware 側の二重で安全範囲クランプがかかります。

| 軸 | Bridge 側デフォルト安全範囲 | Firmware 側安全範囲 |
|---|---|---|
| X | -300 〜 300 | -450 〜 450 |
| Y | 0 〜 450 | 0 〜 450 |

---

## 3. 入力イベント

### 概要

StackChan 本体への物理的な働きかけ（タッチ、ボタン、動き）を検出し、外部システムが取得できる形で提供します。

イベントは Firmware 側の **固定長キュー** に蓄積され、Bridge 経由で外部システムがポーリング取得します。

```text
StackChan タッチパネル
  │ タップ検出
  ▼
InputController（Firmware）
  │ InputEvent をキューに追加
  ▼
Bridge  GET /events（ポーリング）
  │ Firmware から "EVENTS\n" で取得
  │ JSON に変換して返す
  ▼
外部システム
```

### イベント種別

| type | 発生源 | value | 補足フィールド |
|---|---|---|---|
| `touch` | 画面タッチ | `tap` | `x`, `y`（座標） |
| `button` | 物理ボタン | `pressed` | — |
| `imu` | 加速度/ジャイロ | `moved`, `shake` | — |

### イベント構造

```json
{
  "id": 1,
  "type": "touch",
  "target": "screen",
  "value": "tap",
  "timestamp": 12345678,
  "x": 123,
  "y": 87
}
```

| フィールド | 説明 |
|---|---|
| `id` | ブートからの通し番号 |
| `type` | `touch`, `button`, `imu` |
| `target` | 発生源。`screen`, `button_a`, `button_b`, `button_c`, `device` |
| `value` | イベントの値。`tap`, `pressed`, `moved`, `shake` |
| `timestamp` | `millis()` によるタイムスタンプ |
| `x`, `y` | タッチ座標（`type: "touch"` のみ） |

### 取得とクリア

```bash
# 全イベント取得（キューは消えない）
curl http://127.0.0.1:8787/events

# 最新 1 件のみ
curl http://127.0.0.1:8787/events/latest

# キューをクリア
curl -X POST http://127.0.0.1:8787/events/clear
```

`GET /events` は読み取りのみで、キューはクリア**されません**。明示的に `POST /events/clear` を呼ぶ必要があります。

### イベントがない場合

```json
// GET /events
{ "ok": true, "data": { "count": 0, "events": [] } }

// GET /events/latest
{ "ok": true, "data": { "event": null } }
```

### STT イベントとの統合

`events.include_bridge_events: true` の場合、voice-gateway からの STT 結果も `/events` のレスポンスに含まれます。STT イベントの詳細は [audio-pipeline.md](audio-pipeline.md) を参照。

---

## 状態確認

現在の身体状態は `/status` で取得できます。

```bash
curl http://127.0.0.1:8787/status
```

```json
{
  "ok": true,
  "data": {
    "connected": true,
    "mode": "Ready",
    "expression": "Neutral",
    "mood": "Calm",
    "pose": "Neutral",
    "x": 0, "y": 0,
    "speaking": false,
    "blinking": false,
    "eventCount": 0,
    "touchActive": false,
    "buttonActive": false,
    "imuMoving": false
  }
}
```

`/status` は Bridge がキャッシュしている最終既知状態を返します。Firmware への都度問い合わせは行いません。
