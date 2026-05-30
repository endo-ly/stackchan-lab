# 音声パイプライン

音声の出力（TTS 再生）と入力（STT / Wake Word）の全体像を説明します。

**核心設計**: 音声データ（WAV）は **voice-gateway** と **StackChan 本体** が Wi-Fi 上で直接やり取りします。Bridge は設定管理と非音声系 API のみを担当し、音声バイナリ自体は中継しません。

## 全体構造

```text
┌─────────────────────────────────────────────────────────────────────┐
│                     voice-gateway（独立サービス）                     │
│                       Python / FastAPI / 8012                       │
│                                                                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                │
│  │  TTS Provider│  │  STT Provider│  │ 共通エンドポイント│                │
│  │  AivisSpeech │  │ ReazonSpeech │  │ /health      │                │
│  │              │  │  K2 v2       │  │ /v1/models   │                │
│  │  (他)        │  │  (他)        │  │ /v1/capabilities│               │
│  └──────┬──────┘  └──────┬──────┘  └─────────────┘                │
│         │                │                                          │
│  /v1/audio/speech  /v1/transcribe                                   │
│  OpenAI互換TTS     multipart/form-data                              │
│                                                                     │
└─────────┬────────────────┬──────────────────────────────────────────┘
          │ TTSレスポンス   │ STTアップロード
          │ (WAVバイナリ)   │ (WAV multipart)
          ▼                ▼
┌──────────────────────┐  ┌──────────────────────┐
│ 外部エージェント/システム│  │ StackChan 本体       │
│ (LLM/会話エンジン)     │  │ (ESP32-S3 / Wi-Fi)   │
│                      │  │                      │
│ POST /play-wav ──────┼──┼─▶ AudioController     │
│ (WAV直接送信)         │  │ ・独自WAVパーサ       │
│                      │  │ ・playRaw()直接再生   │
└──────────────────────┘  │                      │
                          │ ◀─── POST multipart ─┘
                          │       voice-gatewayへ
                          │       マイク録音WAV送信
                          └──────────────────────┘
                                   │
                                   │ 設定・状態管理
                                   │（USB Serial / Wi-Fi HTTP）
                                   ▼
                          ┌──────────────────────┐
                          │ stackchan-bridge      │
                          │ (Node.js / 8787)      │
                          │                       │
                          │ ・デバイス設定転送    │
                          │ ・/face /led /pose   │
                          │ ・イベントポーリング  │
                          └──────────────────────┘
```

---

## 1. 音声出力（TTS → WAV 再生）

外部エージェントが voice-gateway で TTS 生成し、生成された WAV を StackChan 本体に直接再生させる経路です。

### フロー

```text
外部エージェント / 会話システム
  │ ① POST /v1/audio/speech → voice-gateway
  │    {"input":"こんにちは、スタックチャンです", "voice":"your-voice"}
  ▼
voice-gateway
  │ TTS Provider（AivisSpeech / Irodori-TTS 等）で音声合成
  │ → WAV生成（PCM mono 16bit、任意の有効サンプルレート）
  ◀── ② レスポンス: WAVバイナリ
  ▼
外部エージェント / curl
  │ ③ POST /play-wav → StackChan本体IP:80（Wi-Fi直接）
  │    RAW body: WAVバイナリ
  │    Header: X-StackChan-Token
  ▼
StackChan Firmware
  │ HttpServerController::handlePlayWavBody()
  │   RAW body handlerでバイナリ受信（max 1MB）
  │   → body_.prepareWav() → memcpy → body_.queuePreparedWav()
  ▼
AudioController
  │ parseWavPcm16Mono()で独自パース
  │   検証: RIFF/WAVE, fmt(audioFormat=1, channels=1, bitsPerSample=16)
  │         dataチャンク抽出
  │ queuePlayWav() → Queued状態
  ▼
main loop
  │ processAudioQueue()
    │ enterPlaybackMode()
      │ wake停止 → mic録音終了待ち → mic_.end() → Speaker.begin(48000)
    │ startQueuedPlay()
      │ M5.Speaker.playRaw(int16_t*, samples, wavSampleRate, ...)
      │ M5Unified内部WAVパーサをバイパス
  ▼
スピーカー出力（I2S / AW88298）
```

### voice-gateway TTS API

| エンドポイント | 形式 | 用途 |
|---|---|---|
| `POST /v1/audio/speech` | OpenAI互換 | エージェント・外部ツール向け |
| `POST /v1/speech` | Native（拡張パラメータ） | 詳細制御が必要な場合 |

```bash
# OpenAI互換TTS
curl -X POST http://<voice-gateway>:8012/v1/audio/speech \
  -H "Content-Type: application/json" \
  -d '{"model":"tts-default","voice":"your-voice","input":"こんにちは"}' \
  --output speech.wav
```

### StackChan 直接再生 API

| エンドポイント | 説明 |
|---|---|
| `POST /play-wav` | WAVバイナリ直接送信・再生（RAW body）。StackChan IP に直接送信 |
| `GET /audio/status` | 再生状態の取得 |
| `POST /audio/volume` | 音量設定（0-255） |
| `POST /audio/stop` | 再生停止 |

### 対応 WAV 形式

| 項目 | 制約 |
|---|---|
| コーデック | PCM（非圧縮、audioFormat = 1） |
| チャンネル | モノラル（channels = 1） |
| ビット深度 | 16-bit |
| サンプルレート | 任意の有効レート（16000Hz、24000Hz、44100Hz 等） |
| 最大サイズ | 1MB（Firmware 側でチェック） |
| I2S クロック | 固定 48000Hz（M5Unified が自動リサンプル） |

### 状態遷移

```text
Idle → Receiving → Queued → Playing → Finished → Idle
  │                                    │
  └────────────────────────────────────┘
                Error / Stop
```

---

## 2. 音声入力（STT パイプライン）

StackChan 本体のマイクで録音し、voice-gateway の STT 機能でテキスト化する経路です。音声データは StackChan → voice-gateway へ直接送信されます。

### アーキテクチャ上の設計判断

- **STT 推論は voice-gateway 上で実行**。Firmware や Bridge に推論ロジックは含めない。
- **直接通信**: StackChan 本体が voice-gateway の `/v1/transcribe` に multipart 形式で WAV を直接アップロード。
- Bridge は `speechServicesUrl` の設定管理のみ。音声バイナリは中継しない。

### 経路 A：手動録音（`POST /mic/record`）

```text
外部システム / curl
  │ POST /mic/record {"durationMs":3000} → StackChan本体（Wi-Fi HTTP）
  ▼
StackChan Firmware  MicController
  │ M5Unified 経由で 16kHz mono 録音
  │ メモリ上で WAV 生成（PCM 16kHz mono 16bit）
  │ POST multipart/form-data → voice-gateway /v1/transcribe
  │   （speechServicesUrl に設定されたURLへ直接送信）
  ▼
voice-gateway
  │ STT Provider（ReazonSpeech K2 v2 等）で文字起こし
  │ → テキスト結果をレスポンスボディで返却
  ◀── レスポンス
  ▼
StackChan Firmware
  │ レスポンス文字列を lastSpeechResponse に保存
外部システム
  │ GET /wake/status → wakeUpload.lastSpeechResponse で確認
```

### 経路 B：Wake Word 自動検出・録音

```text
StackChan Firmware  WakeController
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
検出後処理（processWakeDetection）
  │ 顔・LED フィードバック（alert プリセット）
  │ 3 秒間録音
  │ POST multipart/form-data → voice-gateway /v1/transcribe
  │   （speechServicesUrl へ直接アップロード）
  ▼
voice-gateway
  │ STT処理 → テキスト結果を返却
  ◀── レスポンス
  ▼
StackChan Firmware
  │ lastSpeechResponse に保存
  │ アップロード完了後、待受を自動再開
```

### voice-gateway STT API

| エンドポイント | 形式 | 用途 |
|---|---|---|
| `POST /v1/transcribe` | Native multipart | StackChan 本体からの直接アップロード |
| `POST /v1/audio/transcriptions` | OpenAI互換 | 外部ツール・エージェント向け |
| `GET /v1/transcribe/latest` | — | 直近の転写結果（確認用） |

```bash
# Native STT（StackChan本体が使用）
curl -X POST http://<voice-gateway>:8012/v1/transcribe \
  -F "file=@audio.wav" \
  -F "model=stt-default" \
  -F "source=stackchan"
```

### 結果の確認方法

| 格納先 | 取得方法 |
|---|---|
| `lastSpeechResponse` | `GET /wake/status` の `data.wakeUpload.lastSpeechResponse` |

### 関連 API（StackChan 本体 Wi-Fi HTTP）

| エンドポイント | 説明 |
|---|---|
| `POST /mic/record` | 手動録音・voice-gateway へ直接アップロード |
| `GET /mic/status` | マイク状態・最終録音情報 |
| `GET /wake/status` | lastSpeechResponse の確認 |

---

## 3. Wake Word（常時待受音声入力）

StackChan 本体のマイクで常時待受を行い、Wake Word 検出後に自動録音・voice-gateway へのアップロードを行う機能です。

### 設計判断

| 判断 | 理由 |
|---|---|
| **Firmware 側で検出** | PC/Bridge 側マイク前提にすると StackChan 単体の音声入力デバイスとして成立しない |
| **microWakeWord 方式を採用** | ESP32-S3 で現実的なサイズ、既存 Arduino firmware を壊さず段階導入可能 |
| **ESP-SR（WakeNet）不採用** | PlatformIO + Arduino 構成との統合に ESP-IDF component 依存の問題あり |
| **Porcupine 不採用** | AccessKey 必須・ライセンス条件がプロジェクトの自前志向と合わない |
| **openWakeWord 不採用** | ESP32-S3 常駐には重い |

### Wake Word モデル

| 項目 | 現在 | 目標 |
|---|---|---|
| モデル | `okay_nabu` | `スタックチャン` 専用モデル |
| Wake Word | "Okay Nabu"（英語） | 「スタックチャン」（日本語） |
| モデルサイズ | 60,264 バイト | 未定 |
| 確度閾値 | 0.87 | 専用モデルで再調整 |
| Sliding window | 5 | 専用モデルで再調整 |

`okay_nabu` は engine bring-up 用の公開モデルであり、最終的な `スタックチャン` Wake Word ではありません。

### 状態管理

`POST /wake/start` 後は基本的に待受状態が継続します。`/wake/start` は `wakeAutoStart=true` を永続保存し、次回起動後も Wi-Fi 接続と `speechServicesUrl` 設定が揃っていれば自動で待受を開始します。`/wake/stop` は `wakeAutoStart=false` を保存します。

```text
Idle → Detecting → WakeDetected → RecordingCommand → Uploading → Detecting
```

停止条件:

- `POST /wake/stop`
- Wake Word 検出（検出後処理完了後、自動再開）
- マイク録音エラー
- 推論エラー
- 手動 `/mic/record` 使用時（排他制御）

### 関連 API（StackChan 本体 Wi-Fi HTTP）

| エンドポイント | 説明 |
|---|---|
| `GET /wake/status` | Wake Word 状態（デバッグ情報・lastSpeechResponse 含む） |
| `POST /wake/start` | Wake Word 待受開始 |
| `POST /wake/stop` | Wake Word 待受停止 |

---

## 4. エンドツーエンド音声応答フロー

Wake Word 検出から STT → 固定応答 TTS → 再生までの自動フローです。bridge は会話 runtime を持たず、STT callback をきっかけに固定応答を音声再生します。

```text
[1] Wake Word 検出（StackChan Firmware）
  │ "Okay Nabu"
  ▼
[2] 自動録音（wake.record_duration_ms）
  ▼
[3] STT アップロード（StackChan → voice-gateway）
  │ POST multipart/form-data → /v1/transcribe
  ▼
[4] STT 結果取得（voice-gateway → StackChan）
  │ レスポンスボディ: "こんにちは、スタックチャン"
  │ StackChan: lastSpeechResponse に保存
  ▼
[5] STT callback（voice-gateway → bridge）
  │ POST /stt/events
  ▼
[6] 固定応答テキスト生成（bridge）
  │ "はい、聞こえています。"
  ▼
[7] TTS 生成（bridge → voice-gateway）
  │ POST /v1/audio/speech → voice-gateway
  │ テキスト → WAV
  ◀── WAVバイナリ
  ▼
[8] 音声再生（bridge → StackChan）
  │ POST /play-wav（WAVバイナリ直接送信）
  ▼
[9] 発話完了
  │ 顔: 口パク終了 / LED: mood 復帰
```

固定応答テキストの生成部分だけを後で LLM や外部会話システムに置き換えられます。

---

## 5. 設定の流れ

### 1. voice-gateway のセットアップ

voice-gateway を起動し、TTS・STT Provider を設定します。

```bash
cd voice-gateway
uv sync --group dev --extra reazonspeech-k2
export VOICE_GATEWAY_MODE=all
export AIVIS_MANAGE_ENGINE=true
uv run uvicorn app.main:app --host 0.0.0.0 --port 8012
```

### 2. StackChan 本体へのデバイス設定

Bridge CLI で `speechServicesUrl`（voice-gateway の URL）を StackChan 本体に送信します。

```bash
cd stackchan-lab/bridge
# config.yaml に voice_gateway.base_url を設定
# 例: voice_gateway.base_url: "http://192.168.0.210:8012"
# wake.record_duration_ms で Wake 後録音時間を設定
npm run device:config -- config.yaml
```

内部では USB Serial 経由で `DEVICE:CONFIG_JSON` コマンドが送信され、StackChan は `speechServicesUrl` と Wake 録音時間をフラッシュに保存します。Wake Word / マイク録音時は `voice_gateway.base_url` から組み立てた `/v1/transcribe` に直接アップロードします。

### 3. 外部エージェントからの利用

外部 AI エージェントが音声を再生する場合:

```bash
# ① TTS生成（voice-gateway経由）
curl -X POST http://<voice-gateway>:8012/v1/audio/speech \
  -H "Content-Type: application/json" \
  -d '{"input":"こんにちは","voice":"your-voice"}' \
  --output speech.wav

# ② 直接StackChanに再生させる
curl -X POST http://<stackchan-ip>:80/play-wav \
  -H "X-StackChan-Token: <token>" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @speech.wav
```

---

## 6. 制限事項と将来計画

| 項目 | 状態 |
|---|---|
| ストリーミング STT | 未実装。現在は録音完了後の一括転写のみ |
| スタックチャン専用 Wake Word モデル | 未着手。`okay_nabu` で基盤確認済み |
| Wake Word 経路の非同期状態機械化 | 検討中。現在は同期的処理 |
| カメラ画像認識 | 未実装。エンドポイントは予約済み（501 返却） |
| WebSocket / SSE によるイベントプッシュ | 未実装 |
| エンドツーエンド自動会話（LLM 連携） | 外部システム側で実装が必要 |
| エージェント認証・権限制御 | 検討中。現状は StackChan Token を直接扱う |
