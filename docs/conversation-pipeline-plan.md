# 音声応答パイプライン実装 Plan

Wake Word、STT、TTS、StackChan 再生を一括で動かすための実装計画。

まずは STT 結果に対して固定文字列を返し、その文字列を voice-gateway で TTS 生成して StackChan に再生させる。固定文字列を作る部分だけを、後で LLM や外部会話システムに置き換えられるようにしておく。

## 目的

最初に実現するフローはこれだけに絞る。

```text
StackChan Firmware
  Wake Word 検出
  -> 音声録音
  -> voice-gateway /v1/transcribe へ直接アップロード

voice-gateway
  ReazonSpeech などで STT
  -> bridge /stt/events へ callback

stackchan-bridge
  STT 結果を受け取る
  -> 固定文字列を返答テキストとして作る
  -> voice-gateway /v1/audio/speech で WAV 生成
  -> StackChan /play-wav で再生
```

この時点では、文脈管理、人格、LLM、長期記憶、外部会話 runtime は扱わない。

## 責務分離

| 層 | 持つ責務 | 持たない責務 |
|---|---|---|
| `voice-gateway` | STT/TTS Provider 抽象、Aivis/Reazon の実行差異吸収、`/v1/transcribe`、`/v1/audio/speech` | Wake Word、StackChan 再生制御、会話判断 |
| StackChan Firmware | Wake Word、録音、STT 直接アップロード、WAV 再生、身体 I/O | TTS Provider 固有知識、返答生成、会話状態 |
| `stackchan-bridge` | STT callback 受信、TTS API 呼び出し、StackChan 再生指示、状態確認 | 会話 runtime、長期状態、Provider 固有処理 |

bridge に置くのは、会話ではなく **音声応答パイプライン**。STT event を受け取って、固定テキストを WAV にして再生する単発処理である。

## 設計判断

### STT 結果の入口は callback にする

`GET /wake/status` の `lastSpeechResponse` を polling する方法もあるが、主経路にはしない。

理由:

- 状態値 polling は重複処理や取りこぼし対策が必要になる。
- voice-gateway には `STT_CALLBACK_URL` が既にある。
- bridge には `POST /stt/events` が既にある。

主経路:

```text
StackChan -> voice-gateway /v1/transcribe
voice-gateway -> bridge /stt/events
bridge -> 音声応答パイプライン
```

### bridge は callback を待たせない

`/stt/events` は STT 結果の保存とパイプライン起動だけを行い、すぐ 200 を返す。

TTS 生成と再生は時間がかかるので、callback response の中で待たない。

```text
POST /stt/events
  -> payload validation
  -> BridgeState / BridgeEventStore 更新
  -> SpokenReplyPipeline.handle(result) を非同期起動
  -> HTTP 200

SpokenReplyPipeline
  -> fixed reply text
  -> voice-gateway TTS
  -> DeviceTransport.playWav()
```

これは会話 runtime ではなく、callback 後の副作用を実行するための小さな非同期処理である。

### 固定文字列部分だけを小さく切る

固定返答は config ではなくコードに置く。

例:

```ts
function buildReplyText(): string {
  return "はい、聞こえています。";
}
```

後で LLM や外部会話システムに接続するときは、この `buildReplyText()` 相当の部分だけを置き換える。今の段階で `Agent` interface や外部システム adapter は作らない。

### TTS は voice-gateway 経由に限定する

bridge は AivisSpeech Engine を直接呼ばない。

```text
SpokenReplyPipeline
  -> VoiceGatewayClient.synthesizeSpeech()
    -> POST /v1/audio/speech
```

Aivis か別 TTS Provider かは voice-gateway の model/voice 設定で吸収する。

### 再生は DeviceTransport 経由にする

StackChan への WAV 再生は `DeviceTransport.playWav()` を使う。

Wi-Fi 直接再生が主経路だが、bridge 内の処理は Serial/Wi-Fi の差を知らない。

## 追加する実装の形

余計な階層を増やさず、必要な concrete module だけを追加する。

```text
bridge/src/voiceGateway/
  VoiceGatewayClient.ts

bridge/src/spokenReply/
  SpokenReplyPipeline.ts
```

役割:

| file | 責務 |
|---|---|
| `VoiceGatewayClient.ts` | voice-gateway の `/v1/audio/speech` を呼び、WAV bytes を返す |
| `SpokenReplyPipeline.ts` | STT result -> fixed reply text -> TTS -> playback を実行する |

`SpokenReplyPipeline` は `DeviceTransport` と `VoiceGatewayClient` を受け取るだけにする。

```text
SpokenReplyPipeline
  input: TranscriptionResult
  deps:
    - VoiceGatewayClient
    - DeviceTransport
  behavior:
    - source filter
    - busy guard
    - fixed reply text
    - synthesize
    - playWav
```

## 設定方針

固定応答テキストは config に出さない。

config に置くのは、接続先、TTS model/voice、パイプラインの有効/無効、Wake 録音時間など、環境や運用で変わるものだけにする。

目標形:

```yaml
voice_gateway:
  base_url: "http://192.168.0.210:8012"
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
```

補足:

- `voice_gateway.base_url` から `/v1/transcribe` と `/v1/audio/speech` を組み立てる。
- StackChan Firmware には、bridge の device config 経由で `speechServicesUrl = voice_gateway.base_url + /v1/transcribe` を渡す。
- voice-gateway から bridge への callback は voice-gateway 側 `.env` の `STT_CALLBACK_URL=http://<bridge>:8787/stt/events` で管理する。
- `spoken_reply.listen_sources` で Wake 経由の STT だけを対象にする。手動録音や smoke test 由来の STT に勝手に返答しないため。
- `busy_policy` は最初は `ignore` のみ対応でよい。

## Wake 後録音時間の一般化

変更前の Wake 後録音は Firmware 側で `recordMicWav(3000)` と固定されていた。

これは STT Provider の設定ではなく、Wake 後にユーザー発話を何秒切り出すかというデバイス入力ポリシーなので、`wake.record_duration_ms` として bridge config から device config に渡す。

device config:

```json
{
  "speechServicesUrl": "http://192.168.0.210:8012/v1/transcribe",
  "wakeAutoStart": true,
  "wakeRecordDurationMs": 5000
}
```

Firmware 側では安全範囲を持つ。

| 項目 | 値 |
|---|---|
| default | 5000ms |
| min | 1000ms |
| max | 15000ms |

最初は `record_duration_ms` だけでよい。VAD や無音終端は今は設計に入れない。

## 状態確認 API

本番運用で詰まり箇所を見られるように、bridge に状態確認を追加する。

| Method | Path | 用途 |
|---|---|---|
| `GET` | `/spoken-reply/status` | enabled、busy、last input、last reply、last error |
| `POST` | `/spoken-reply/start` | bridge 上の自動音声応答を有効化 |
| `POST` | `/spoken-reply/stop` | bridge 上の自動音声応答を無効化 |

これは Firmware の `/wake/start` `/wake/stop` とは別物。

- `/wake/start`: StackChan 本体の Wake Word 待受を開始する。
- `/spoken-reply/start`: bridge が STT callback に自動応答する。

## 実装 Plan

### Phase 1: Wake 録音時間を設定化する

目的:

- Wake 後 3 秒固定をやめる。
- 起動後自動 Wake と同じ device config の流れで保存する。

作業:

1. `bridge/src/config/types.ts` に `wake.recordDurationMs` を追加する。
2. `bridge/src/config/loadConfig.ts` で `wake.record_duration_ms` を読む。
3. `bridge/config.example.yaml` に `wake.record_duration_ms` を追加する。
4. `bridge/src/cli/deviceConfig.ts` が `wakeRecordDurationMs` を送る。
5. Firmware の `WiFiConfig` に `wakeRecordDurationMs` を追加する。
6. Preferences に保存/読み込みを追加する。
7. `CommandHandler::completeDeviceConfigJsonTransfer()` で `wakeRecordDurationMs` を受け取る。
8. `HttpServerController::processWakeDetection()` の固定録音時間を設定値に置き換える。

受け入れ条件:

- `wake.record_duration_ms: 5000` で Wake 後録音が 5 秒になる。
- 未設定時は 5000ms になる。
- 1000ms 未満、15000ms 超過で危険な録音時間にならない。
- `/wake/start` `/wake/stop` は永続設定を書き換えない。

### Phase 2: voice-gateway client を追加する

目的:

- bridge から TTS を呼ぶための concrete client を用意する。
- Aivis API は直接扱わない。

作業:

1. `bridge/src/voiceGateway/VoiceGatewayClient.ts` を追加する。
2. `POST /v1/audio/speech` を呼ぶ `synthesizeSpeech()` を実装する。
3. `voice_gateway.base_url`、`tts.model`、`tts.voice`、`tts.response_format` を config に追加する。
4. TTS response が WAV bytes であることを `validateWav()` で確認する。

受け入れ条件:

- bridge から voice-gateway 経由で WAV を生成できる。
- bridge は AivisSpeech Engine の URL や API schema を知らない。

### Phase 3: SpokenReplyPipeline を追加する

目的:

- STT result から固定応答音声再生までをつなぐ。
- 会話 runtime や Agent 抽象は作らない。

作業:

1. `bridge/src/spokenReply/SpokenReplyPipeline.ts` を追加する。
2. `handle(result: TranscriptionResult)` を実装する。
3. `listen_sources` に一致しない STT result は無視する。
4. busy 中の二重起動は `busy_policy: ignore` として無視する。
5. `buildReplyText()` で固定文字列を返す。
6. `VoiceGatewayClient.synthesizeSpeech()` で WAV を生成する。
7. `DeviceTransport.playWav()` で StackChan に再生させる。

受け入れ条件:

- `stackchan-wake` source の STT callback を受けると固定応答が再生される。
- 固定応答テキストは config に存在しない。
- 同時に複数 callback が来ても二重再生で崩れない。
- `buildReplyText()` 相当の一点を後で差し替えれば、固定応答以外に変えられる。

### Phase 4: `/stt/events` から非同期起動する

目的:

- voice-gateway の callback を遅延させずに音声応答を開始する。

作業:

1. `main.ts` で `VoiceGatewayClient` と `SpokenReplyPipeline` を組み立てる。
2. `StackChanBridge` または route 登録時に `SpokenReplyPipeline` を渡す。
3. `/stt/events` で STT event 保存後、`void pipeline.handle(result)` を呼ぶ。
4. pipeline 内部で例外を捕捉し、last error に保存する。

受け入れ条件:

- `/stt/events` は TTS/再生完了を待たず 200 を返す。
- TTS 失敗、playback 失敗が `/spoken-reply/status` で分かる。
- bridge の既存 STT event store は壊れない。

### Phase 5: docs を更新する

目的:

- 実装後の正式な起動方法と責務境界を docs に反映する。

作業:

1. `docs/audio-pipeline.md` の Wake -> STT -> TTS -> playback フローを更新する。
2. Wake 録音 3 秒固定の説明を `wake.record_duration_ms` に更新する。
3. `/wake/start` `/wake/stop` が永続設定を書き換えるような記述が残っていれば直す。
4. `docs/architecture.md` の Bridge 層に `spoken reply pipeline` を追加する。
5. `bridge/README.md` に標準起動と config 例を追加する。

受け入れ条件:

- ドキュメント上で、bridge が会話 runtime を持つように読めない。
- Aivis API を知らずに voice-gateway 経由で TTS する構造が分かる。
- Wake、STT、TTS、playback の責務境界が一貫している。

## 標準起動

起動時に追加の環境変数を渡さない。

voice-gateway:

```bash
cd /root/workspace/voice-gateway
uv run uvicorn app.main:app --host 192.168.0.210 --port 8012
```

bridge:

```bash
cd /root/workspace/stackchan-lab/bridge
npm run dev
```

device config:

```bash
cd /root/workspace/stackchan-lab/bridge
npm run device:config -- config.yaml
```

前提:

- voice-gateway の `.env` には `STT_CALLBACK_URL=http://<bridge>:8787/stt/events` を設定しておく。
- bridge の `voice_gateway.base_url` は voice-gateway の URL を指す。
- bridge の `device.transport: wifi` と `wifi.base_url` は StackChan 本体を指す。
- StackChan Firmware は保存済み device config に従って Wake auto start と Wake 録音時間を使う。

## リスクと対策

| リスク | 対策 |
|---|---|
| StackChan 自身の発話を Wake が拾う | Firmware 側 playback 中の Wake 停止に加え、bridge 側で `cooldown_ms` を持つ |
| callback が TTS/再生で遅くなる | `/stt/events` は保存と非同期起動だけで返す |
| 手動録音にも勝手に返答する | `listen_sources` で `stackchan-wake` のみに絞る |
| bridge が会話エンジン化する | 返答生成は固定文字列関数に留め、文脈や人格を持たせない |
| Aivis 固有実装が漏れる | bridge は voice-gateway `/v1/audio/speech` だけを呼ぶ |
| Wake 録音が長すぎる | Firmware 側で `wakeRecordDurationMs` を clamp する |

## 最初にやること

1. Wake 録音時間を `wake.record_duration_ms` として設定化する。
2. `VoiceGatewayClient` を追加して bridge から TTS を呼べるようにする。
3. `SpokenReplyPipeline` を追加し、固定文字列 -> TTS -> playback をつなぐ。
4. `/stt/events` から非同期で pipeline を起動する。
5. 状態 API と docs を整える。

この順であれば、余計な会話抽象を増やさず、まず固定文字列の音声応答を本番導線として成立させられる。
