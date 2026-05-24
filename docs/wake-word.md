# Wake Word Design Note

## 目的

StackChan のマイクで常時待受を行い、wake word 検出後に短時間録音して `stt-adapter` へ送る。

目指す最終形は以下。

```text
StackChan mic
  -> wake word detection on firmware
  -> 3 sec command recording
  -> stt-adapter /transcribe
  -> Bridge /stt/events
```

Wake word 検出は PC / Bridge 側ではなく StackChan 本体側で行う。
理由は、実運用で StackChan を単体の音声入力デバイスとして扱いたいため。
Bridge は wake word 音声を受け取らず、STT 完了後の event を受け取るだけにする。

## 候補

### ESP-SR / WakeNet

Espressif 公式寄りの音声認識 / wake word エンジン。
ESP32-S3 を対象にした WakeNet があり、CoreS3 のハードウェアとは方向性が合う。

ただし、ESP-SR は ESP-IDF component として使う前提が強い。
現在の firmware は PlatformIO + Arduino framework で構成しているため、`lib_deps` に GitHub URL を足すだけでは成立しなかった。

確認時の問題:

```text
fatal error: esp_mn_speech_commands.h: No such file or directory
```

これは単純な include 漏れではなく、ESP-IDF component、Kconfig、model component、include path が揃っていないことが原因。

ESP-SR を採用するには、少なくとも以下のどちらかが必要になる。

- PlatformIO を `framework = arduino, espidf` の混合構成へ寄せる
- firmware 全体を ESP-IDF 寄りへ再構成し、Arduino を component として扱う

これは wake word だけの局所変更ではなく、firmware 基盤の変更になる。
そのため Phase 9.5 では採用しない。

### microWakeWord

ESP32 系で使われている小さな wake word モデル方式。
特徴量生成と TFLite Micro の INT8 モデルで検出する。
ESPHome で実績があり、カスタム wake word を作る方向にも比較的つなげやすい。

現在の Arduino firmware に直接入れるために必要だった要素:

- `esp-micro-speech-features`
- `MicroTFLite`
- microWakeWord 形式の `.tflite` model
- model manifest 相当の設定値
- 16kHz mono PCM を短い block で継続取得する処理
- TFLite tensor arena / resource variable の管理
- `/wake/start`, `/wake/stop`, `/wake/status`

採用理由:

- PC 側 wake word に寄せず、本体内で完結できる
- ESP32-S3 で現実的なサイズに収まる
- 既存 Arduino firmware を大きく壊さず段階導入できる
- 将来 `スタックチャン` 専用モデルを作る道筋がある

注意点:

- 「ライブラリを1つ足せば終わり」ではない
- 公開済みモデルにない wake word は専用学習が必要
- モデルごとに threshold、sliding window、tensor arena size が異なる
- 実機マイク音量、特徴量スケール、推論状態管理の調整が必要

### Porcupine

Picovoice の wake word エンジン。
組み込み用途の実績は強い。

採用しなかった理由:

- AccessKey が必要
- ライセンス条件がこのプロジェクトのローカル・自前志向と少し合いにくい
- ESP32-S3 / CoreS3 での現行 firmware との統合確認コストがある
- `スタックチャン` の日本語カスタム wake word を扱う場合も、結局モデル作成・契約・運用の検討が必要

### openWakeWord

PC / サーバー側で有力な wake word エンジン。

採用しなかった理由:

- ESP32-S3 本体常駐には重い
- Bridge / PC 側マイク前提に寄ると、StackChan 本体で完結する設計から外れる
- 今回の目的は「StackChan 側のマイクで wake word を検出すること」

## 採用方針

Phase 9.5 では microWakeWord 方式を採用する。

ただし、現時点で同梱しているモデルは `okay_nabu` であり、これは engine bring-up 用の公開モデルである。
最終的な `スタックチャン` wake word ではない。

```text
current model: okay_nabu
current wake word: Okay Nabu
final desired wake word: スタックチャン
```

公開済みモデルを確認した範囲では、`StackChan` / `スタックチャン` の microWakeWord model は見つかっていない。
したがって、`スタックチャン` で起動するには専用モデルの作成・学習・評価が必要。

表示名だけ `スタックチャン` に変えることはしない。
それは実際には `Okay Nabu` を聞いているだけになり、wake word 実装として不正確だから。

## 現在の実装

Firmware 側に以下を追加している。

- `WakeState`
- `WakeController`
- `WakeModelData`
- `GET /wake/status`
- `POST /wake/start`
- `POST /wake/stop`
- 16kHz / 10ms block のマイク取得
- wake 用入力 gain
- `esp-micro-speech-features` による特徴量生成
- `MicroTFLite` による推論
- wake 検出後の3秒録音
- `stt-adapter` への multipart WAV upload
- 検出時の顔・LEDフィードバック

`POST /wake/start` 後は、基本的に待受状態が継続する。

```text
/wake/start
  -> mic block recording
  -> feature generation
  -> TFLite inference
  -> wake detection
  -> 3 sec recording
  -> upload to stt-adapter
  -> restart wake waiting
```

停止する条件:

- `POST /wake/stop`
- wake word 検出
- マイク録音エラー
- 推論エラー
- 手動 `/mic/record` などで録音経路を使う場合

wake word 検出後は自動録音と upload 後、再度 wake 待受へ戻る。

## `/wake/status`

通常状態に加えて、wake word tuning 用の `debug` を返す。
これは一時ログではなく、今後の専用モデル作成・実機調整でも役に立つため残す。

例:

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
        "recordingState": 0
      },
      "features": {
        "min": 0,
        "max": 0
      },
      "model": {
        "lastRawOutput": 0,
        "maxRawOutput": 0
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

debug の見方:

| Field | 意味 |
| --- | --- |
| `queuedBlocks` | wake 用にマイク録音blockを投入した回数 |
| `processedBlocks` | 録音完了後、処理したblock数 |
| `featureFrames` | 特徴量が生成された回数 |
| `inferenceRuns` | TFLite推論が実行された回数 |
| `lastSamplesConsumed` | frontend が最後に消費したsample数 |
| `mic.rms` | wake 用入力gain後の音量目安 |
| `mic.peak` | wake 用入力gain後のピーク値 |
| `mic.recordingState` | M5Unified Mic の録音状態 |
| `features.min/max` | モデル入力へ入る特徴量のレンジ |
| `model.lastRawOutput` | 最新推論の raw output |
| `model.maxRawOutput` | `/wake/start` 後の最大 raw output |

## 実機確認結果

確認できていること:

- `/wake/start` で `running: true` になる
- mic block は継続取得される
- feature generation は進む
- TFLite inference は進む
- wake 用入力gain後の `mic.rms` / `mic.peak` は上がる
- `model.maxRawOutput` は 0 ではなく、モデルが完全に死んでいるわけではない

確認時の例:

```json
{
  "queuedBlocks": 265,
  "processedBlocks": 264,
  "featureFrames": 262,
  "inferenceRuns": 87,
  "lastSamplesConsumed": 160,
  "mic": {
    "rms": 464,
    "peak": 1424,
    "recordingState": 0
  },
  "features": {
    "min": -128,
    "max": -50
  },
  "model": {
    "lastRawOutput": 0,
    "maxRawOutput": 5
  }
}
```

まだ確認できていないこと:

- `Okay Nabu` の安定検出
- wake 検出後の3秒録音から `stt-adapter` upload までの実機成功
- `スタックチャン` wake word

現状の解釈:

- マイク取得、特徴量生成、推論ループは動いている
- ただし `okay_nabu` model の出力がかなり弱い
- 原因候補は、発話・マイク位置・特徴量スケール・モデルの相性・streaming state の扱い
- `スタックチャン` については公開モデルがないため、別途モデル作成が必要

## 今後

### 1. `okay_nabu` の検出確認

現状の microWakeWord 経路が実機で検出まで行けるかを確認する。
ただし `Okay Nabu` は最終目的ではないため、ここに時間を使いすぎない。

確認候補:

- ESPHome 実装との差分確認
- feature scaling の再確認
- model input stride / resource variable の扱い確認
- wake 用gainの調整
- 発話距離・向きの調整

### 2. `スタックチャン` 専用モデル

本命。

必要になるもの:

- positive sample: 「スタックチャン」の発話音声
- negative sample: 似ているが違う音、雑音、通常会話
- training pipeline
- `.tflite` model
- probability cutoff
- sliding window size
- tensor arena size
- 実機での誤検出 / 未検出評価

`スタックチャン` 専用モデルができたら、`WakeModelData` と manifest相当の定数を差し替える。
モデル差し替えは動作仕様そのものが変わるため、必ず明示的に確認してから行う。

### 3. 状態機械化

現在の wake 検出後処理は同期的。
安定性や応答性に問題が出る場合は、以下のような状態機械へ分ける。

```text
Idle
  -> Detecting
  -> WakeDetected
  -> RecordingCommand
  -> Uploading
  -> RestartingWake
  -> Detecting
```

## 参考

- ESP-SR WakeNet: https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/wake_word_engine/README.html
- ESP-SR Getting Started: https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/getting_started/readme.html
- microWakeWord: https://microwakeword.com/
- microWakeWord models: https://github.com/esphome/micro-wake-word-models
- ESPHome micro_wake_word: https://esphome.io/components/micro_wake_word/
- Picovoice Porcupine: https://picovoice.ai/docs/
