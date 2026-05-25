# 音声パイプライン

音声の出力（TTS 再生）と入力（STT / Wake Word）の全体像を説明します。

## 全体構造

```text
  ┌─────────────────┐     ┌──────────────────┐
  │  外部システム     │────▶│   TTS エンジン    │
  │ (会話エンジン)    │     │  テキスト → WAV   │
  └────────┬─────────┘     └────────┬─────────┘
           │◀──── WAV バイナリ ──────┘
           │
           │ POST /play-wav (WAV)
           ▼
┌──────────────────────────────────────────────────────┐
│                  stackchan-bridge                     │
│                                                      │
│  POST /play-wav  ─────▶ Firmware へ WAV 転送          │
│  POST /stt/events ◀─── stt-adapter から受信          │
│  GET  /stt/latest ────▶ 外部システムへテキスト提供     │
└──────┬───────────────────────────────────────────────┘
       │ USB Serial / Wi-Fi
       ▼
┌──────────────┐    ┌──────────────────────┐
│  StackChan   │    │     stt-adapter       │
│  Firmware    │    │  (Python / ReazonSpeech│
│              │    │   K2 v2 等)            │
│  WAV再生     │    │                       │
│  スピーカー   │    │  /transcribe          │
│              │    │  → テキスト化          │
│  マイク録音   │───▶│  WAV アップロード      │
│  Wake Word   │    └──────────┬───────────┘
│  常時待受     │               │
└──────────────┘               │ POST /stt/events
                               │ (Bridge へ callback)
```

パイプラインは **出力（下り）** と **入力（上り）** の 2 系統に大別されます。入力はさらに「手動録音」と「Wake Word 自動録音」の 2 経路があります。

---

## 1. 音声出力（TTS → WAV 再生）

外部システムがテキストから生成した WAV を、StackChan のスピーカーで再生する経路です。

### フロー

```text
TTS エンジン
  │ テキスト → WAV（PCM 16kHz mono 16bit）
  ▼
Bridge  POST /play-wav（multipart/form-data）
  │ WAV バリデーション（形式・サイズ）
  │ Firmware へ転送
  ▼
Firmware  AudioController
  │ WAV 受信 → バッファリング
  │ 再生開始 → 口パク開始（speaking 状態）
  │ 再生完了 → Idle に戻る
  ▼
スピーカー出力
```

### 対応 WAV 形式

| 項目 | 制約 |
|---|---|
| コーデック | PCM（非圧縮） |
| チャンネル | モノラル |
| ビット深度 | 16-bit |
| サンプルレート | 16000Hz または 24000Hz |
| 最大サイズ | 1MB（Bridge 側でチェック） |

### 状態遷移

```text
Idle → Receiving → Playing → Finished → Idle
  │                              │
  └──────────────────────────────┘
            Error
```

`playing` 中は顔の口パクが有効になり、STATUS の `speaking` フラグが `true` になります。再生完了または停止後、`speaking` は `false` に戻ります。

### 関連 API

| エンドポイント | 説明 |
|---|---|
| `POST /play-wav` | WAV 送信・再生 |
| `GET /audio/status` | 再生状態の取得 |
| `POST /audio/volume` | 音量設定（0-255） |
| `POST /audio/stop` | 再生停止 |

詳細は [bridge-api.md](bridge-api.md) を参照。

---

## 2. 音声入力（STT パイプライン）

音声をテキスト化し、外部システムの入力とする経路です。STT 自体は Bridge や Firmware の内部で実行せず、独立した外部サービス **stt-adapter** を使用します。

### アーキテクチャ上の設計判断

- **STT は常に外部サービス**。Bridge や Firmware に STT 推論ロジックは含めない。
- **Bridge は音声の入口にならない**。音声データは直接 stt-adapter へ送られ、Bridge はテキスト化された結果だけを受け取る。
- この分離により、STT エンジンの差し替え（ReazonSpeech → Whisper 等）が Bridge/Firmware に影響しない。

### 経路 A：手動録音（`POST /mic/record`）

```text
外部システム / curl
  │ POST /mic/record {"durationMs":3000}（Wi-Fi HTTP）
  ▼
Firmware  MicController
  │ M5Unified 経由で 16kHz mono 録音
  │ メモリ上で WAV 生成
  │ stt-adapter へ HTTP multipart アップロード
  ▼
stt-adapter  /transcribe
  │ ReazonSpeech K2 v2 等で文字起こし
  │ 結果を Bridge の POST /stt/events へ送信（callback 設定時）
  ▼
Bridge  イベントキューに STT イベントを保存
  │
  ▼
外部システム  GET /stt/latest でテキスト取得
```

### 経路 B：Wake Word 自動録音

経路 A と異なり、トリガーが人間の明示的リクエストではなく、Firmware 上の常時待受による自動検出です。詳細は後述の「Wake Word」セクションを参照。

### STT 結果のイベント統合

stt-adapter からの結果は Bridge の `POST /stt/events` で受信され、以下のように扱われます。

| 格納先 | 取得方法 |
|---|---|
| STT 専用スロット | `GET /stt/latest` |
| イベントキュー（`events.include_bridge_events: true` 時） | `GET /events` |

### 関連 API

| エンドポイント | 説明 |
|---|---|
| `POST /stt/events` | stt-adapter からの転写結果受信（Bridge 側） |
| `GET /stt/latest` | 最新 STT 結果の取得 |
| `POST /mic/record` | Firmware 側マイク録音・アップロード（Wi-Fi HTTP） |

---

## 3. Wake Word（常時待受音声入力）

StackChan 本体のマイクで常時待受を行い、Wake Word 検出後に自動録音・STT 送信を行う機能です。

### 設計判断

| 判断 | 理由 |
|---|---|
| **Firmware 側で検出** | PC/Bridge 側マイク前提にすると StackChan 単体の音声入力デバイスとして成立しない |
| **microWakeWord 方式を採用** | ESP32-S3 で現実的なサイズ、既存 Arduino firmware を壊さず段階導入可能 |
| **ESP-SR（WakeNet）不採用** | PlatformIO + Arduino 構成との統合に ESP-IDF component 依存の問題あり |
| **Porcupine 不採用** | AccessKey 必須・ライセンス条件がプロジェクトの自前志向と合わない |
| **openWakeWord 不採用** | ESP32-S3 常駐には重い |

### 現在の実装

```text
Firmware  WakeController
  │ M5Unified マイクから 16kHz / 160ms chunk を継続取得
  │ 10ms block 単位に分割
  ▼
esp-micro-speech-features
  │ 特徴量生成（40-dim filterbank features）
  ▼
MicroTFLite
  │ TFLite INT8 推論（okay_nabu モデル）
  │ sliding window 平均が閾値（0.87）を超えたら検出
  ▼
検出後処理
  │ 顔・LED フィードバック（alert プリセット）
  │ 3 秒間録音 → stt-adapter へ WAV アップロード
  │ アップロード完了後、待受を自動再開
  ▼
stt-adapter → Bridge /stt/events → 外部システム
```

### Wake Word モデル

| 項目 | 現在 | 目標 |
|---|---|---|
| モデル | `okay_nabu` | `スタックチャン` 専用モデル |
| Wake Word | "Okay Nabu"（英語） | 「スタックチャン」（日本語） |
| モデルサイズ | 60,264 バイト | 未定 |
| 確度閾値 | 0.87 | 専用モデルで再調整 |
| Sliding window | 5 | 専用モデルで再調整 |

`okay_nabu` は engine bring-up 用の公開モデルであり、最終的な `スタックチャン` Wake Word ではありません。`スタックチャン` 専用モデルの作成には以下が必要です。

- 「スタックチャン」の positive 発話サンプル収集
- 類似音・雑音・通常会話の negative サンプル収集
- 学習パイプラインの構築
- `.tflite` モデルの生成
- 実機での誤検出・未検出評価と閾値調整

### 状態管理

`POST /wake/start` 後は基本的に待受状態が継続します。

```text
Idle → Detecting → WakeDetected → RecordingCommand → Uploading → Detecting
```

停止条件:

- `POST /wake/stop`
- Wake Word 検出（検出後処理完了後、自動再開）
- マイク録音エラー
- 推論エラー
- 手動 `/mic/record` 使用時（排他制御）

### デバッグ情報

`GET /wake/status` はチューニング用の `debug` フィールドを返します。

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
    "lastProbability": 0,
    "averageProbability": 0,
    "debug": {
      "queuedBlocks": 0,
      "processedBlocks": 0,
      "featureFrames": 0,
      "inferenceRuns": 0,
      "mic": { "rms": 0, "peak": 0, "maxRms": 0, "maxPeak": 0 },
      "features": { "rawMin": 0, "rawMax": 0, "min": 0, "max": 0 },
      "model": { "lastRawOutput": 0, "maxRawOutput": 0 },
      "detected": { "maxRawOutput": 0, "averageProbability": 0 }
    },
    "wakeUpload": {
      "inProgress": false,
      "lastHttpStatus": 0,
      "lastSpeechResponse": ""
    }
  }
}
```

**デバッグフィールドの見方**

| フィールド | 意味 |
|---|---|
| `queuedBlocks` | マイク録音 chunk を block 換算で投入した数 |
| `processedBlocks` | 10ms 単位で処理した block 数 |
| `featureFrames` | 特徴量が生成された回数 |
| `inferenceRuns` | TFLite 推論が実行された回数 |
| `mic.rms / peak` | 最新 chunk の音量・ピーク値 |
| `mic.maxRms / maxPeak` | 待受セッション内の最大音量 |
| `features.rawMin / rawMax` | frontend 出力のレンジ |
| `features.min / max` | モデル入力特徴量のレンジ |
| `model.lastRawOutput` | 最新推論の raw output |
| `model.maxRawOutput` | `/wake/start` 後の最大 raw output |
| `detected.maxRawOutput` | 最後に検出した時点の最大 raw output |
| `detected.averageProbability` | 最後に検出した時点の sliding window 平均 |

### 関連 API（Firmware 側、Wi-Fi HTTP）

これらのエンドポイントは StackChan 本体の HTTP API です。Bridge 経由ではなく、StackChan の IP アドレスに直接リクエストします（認証には `X-StackChan-Token` ヘッダが必要）。

| エンドポイント | 説明 |
|---|---|
| `GET /wake/status` | Wake Word 状態（デバッグ情報含む） |
| `POST /wake/start` | Wake Word 待受開始 |
| `POST /wake/stop` | Wake Word 待受停止 |
| `GET /mic/status` | マイク状態 |
| `POST /mic/record` | 手動録音・STT 送信 |

---

## STT サービス（stt-adapter）

STT 推論を担当する独立サービスです。リポジトリ外の Python プロジェクトとして管理されます。

### 推奨構成

| 項目 | 推奨 |
|---|---|
| エンジン | ReazonSpeech K2 v2（ONNX） |
| 実行環境 | Python（`reazonspeech` + `sherpa-onnx`） |
| インターフェース | HTTP（`POST /transcribe`） |

### 代替候補（調査メモ）

STT エンジンの選定にあたり以下の候補を検討しました。

| 候補 | 評価 |
|---|---|
| **ReazonSpeech K2 v2** | ✅ 採用。日本語特化、ONNX 形式対応 |
| whisper.cpp | 有力候補。CPU 動作、日本語対応。base モデルでメモリ約 388MB |
| faster-whisper | Python パイプラインとの統合容易。CTranslate2 + int8 量子化 |
| Vosk | 軽量だが日本語の自然会話精度は Whisper 系に劣る |
| sherpa-onnx | ONNX ベースで軽量。日本語モデルの検証が必要 |

---

## 設定の流れ

### 1. stt-adapter のセットアップ（外部リポジトリ）

Python サービスを起動し、`/transcribe` エンドポイントを用意します。

### 2. Bridge の設定

```yaml
# config.yaml
stt:
  transcribe_url: "http://<stt-adapterのIP>:8790/transcribe"

events:
  include_bridge_events: true
```

### 3. StackChan 本体へのデバイス設定転送

Bridge CLI で `stt.transcribe_url` を StackChan 本体に送信します（マイク録音・Wake Word アップロード用）。

```bash
cd bridge
npm run device:config -- config.yaml
```

内部では USB Serial 経由で `DEVICE:CONFIG_JSON` コマンドが送信されます。

---

## 制限事項と将来計画

| 項目 | 状態 |
|---|---|
| ストリーミング STT | 未実装。現在は録音完了後の一括転写のみ |
| スタックチャン専用 Wake Word モデル | 未着手。`okay_nabu` で基盤確認済み |
| Wake Word 経路の非同期状態機械化 | 検討中。現在は同期的処理 |
| カメラ画像認識 | 未実装。エンドポイントは予約済み（501 返却） |
| WebSocket / SSE によるイベントプッシュ | 未実装 |
