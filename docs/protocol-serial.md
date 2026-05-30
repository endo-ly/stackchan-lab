# シリアルプロトコル仕様 v0

## 概要

StackChan Body Firmware を USB Serial 経由で制御するためのテキストプロトコルです。

このプロトコルは、PC やサーバから StackChan の身体制御を行うためのものです。意味理解、会話状態管理、上位アプリ統合は扱いません（それらは Bridge と外部システムの責務です）。

## トランスポート

| 項目 | 値 |
|---|---|
| 物理層 | USB Serial |
| ボーレート | 115200 |
| 形式 | テキスト行ベース |
| 改行 | `\n` または `\r\n` |
| エンコーディング | ASCII 互換テキスト |
| コマンド最大長 | 128 文字 |

## コマンド書式

コマンドは 1 行で送信します。

```text
COMMAND
COMMAND:arg1
COMMAND:arg1:arg2
```

コマンド名は **大文字小文字を区別しません**。

```text
FACE:happy
face:happy
Face:happy
```

## レスポンス書式

- 成功レスポンスは `OK` で始まります。
- エラーレスポンスは `ERR` で始まります。
- `AUDIO:WAV` と `WIFI:SET_JSON` と `DEVICE:CONFIG_JSON` は `READY` レスポンスの後にバイナリ転送フェーズに入ります。
- `EVENTS` は複数行レスポンスになり、最後に `END EVENTS` が返ります。
- デバッグログは `[` で始まります。コマンドのレスポンスと区別してください。

## コマンド一覧

### PING

シリアル接続の疎通確認。

```text
PING
OK PONG
```

---

### VERSION

ファームウェア、プロトコル、ボードのバージョンを返す。

```text
VERSION
OK VERSION firmware=0.8.0 protocol=0.1.0 board=stackchan-cores3
```

---

### HELP

対応コマンドと対応値の一覧を返す。

```text
HELP
OK HELP commands=PING,VERSION,HELP,STATUS,FACE,LED,POSE,MOVE,RESET,AUDIO,EVENTS,WIFI,DEVICE expressions=neutral,happy,sad,angry,sleepy,doubt moods=calm,active,speaking,warning,off poses=neutral,look_left,look_right,look_up,look_down audio=AUDIO:STATUS,AUDIO:VOLUME,AUDIO:STOP,AUDIO:WAV events=EVENTS,EVENTS:LATEST,EVENTS:CLEAR wifi=WIFI:STATUS,WIFI:SET_JSON,WIFI:CONNECT,WIFI:CLEAR device=DEVICE:CONFIG_JSON
```

---

### STATUS

現在の身体状態を返す。

```text
STATUS
OK STATUS mode=Ready expression=Neutral mood=Calm pose=Neutral x=0 y=0 gazeX=0 gazeY=0 speaking=false blinking=false eventCount=0 latestEvent=none latestTarget=none latestValue=none touchActive=false buttonActive=false imuMoving=false
```

**フィールド**

| フィールド | 説明 |
|---|---|
| `mode` | 動作モード（`Ready`） |
| `expression` | 現在の表情 |
| `mood` | 現在の LED ムード |
| `pose` | 現在のポーズ |
| `x` | サーボ X 軸（deci-degree） |
| `y` | サーボ Y 軸（deci-degree） |
| `gazeX` | 視線 X |
| `gazeY` | 視線 Y |
| `speaking` | 発話中か |
| `blinking` | 瞬き中か |
| `eventCount` | イベントキュー内のイベント数 |
| `latestEvent` | 最新イベントの種類 |
| `latestTarget` | 最新イベントの発生源 |
| `latestValue` | 最新イベントの値 |
| `touchActive` | タッチ中か |
| `buttonActive` | ボタン押下中か |
| `imuMoving` | IMU が動きを検出したか |

---

### FACE

顔の表情を変更する。`FACE` は顔そのものの表現に限定されます。`thinking` や `alert` のような複合的な身体表現は、Bridge の `POST /preset` で扱います。

```text
FACE:happy
OK FACE happy
```

**対応値**

- `neutral`
- `happy`
- `sad`
- `angry`
- `sleepy`
- `doubt`

**非対応（プリセットで扱う）**

- `thinking`
- `alert`

**エラー**

```text
ERR MISSING_ARGUMENT expression
ERR INVALID_ARGUMENT expression=thinking
ERR TOO_MANY_ARGUMENTS command=FACE
```

---

### LED

LED のムードを変更する。

```text
LED:calm
OK LED calm
```

**対応値**

- `calm`（穏やか）
- `active`（活動中）
- `speaking`（発話中）
- `warning`（警告）
- `off`（消灯）

**エラー**

```text
ERR MISSING_ARGUMENT mood
ERR INVALID_ARGUMENT mood=blue
ERR TOO_MANY_ARGUMENTS command=LED
```

---

### POSE

プリ定義された安全なポーズに移動する。

```text
POSE:look_left
OK POSE look_left
```

**対応値**

- `neutral`（正面）
- `look_left`（左向き）
- `look_right`（右向き）
- `look_up`（上向き）
- `look_down`（下向き）

**エラー**

```text
ERR MISSING_ARGUMENT pose
ERR INVALID_ARGUMENT pose=spin
ERR TOO_MANY_ARGUMENTS command=POSE
```

---

### MOVE

サーボを X/Y の絶対値で直接移動する。Firmware 側で安全範囲のクランプがかかります。

```text
MOVE:0:0
OK MOVE x=0 y=0
```

値の単位は **deci-degree**（`10 = 1度`）。StackChan-BSP の仕様に準拠します。

現在の安全範囲：

| 軸 | 最小 | 最大 |
|---|---|---|
| X | -450 | 450 |
| Y | 0 | 450 |

範囲外の値は安全範囲内にクランプされ、レスポンスに `clamped=true` が付与されます。

```text
MOVE:999:999
OK MOVE x=450 y=450 clamped=true
```

**エラー**

```text
ERR MISSING_ARGUMENT x,y
ERR INVALID_ARGUMENT x=abc
ERR INVALID_ARGUMENT y=abc
ERR TOO_MANY_ARGUMENTS command=MOVE
```

---

### RESET

身体を安全なニュートラル状態に戻す。ESP32 の再起動ではありません。

```text
RESET
OK RESET
```

リセット時の動作：

- 表情: `neutral`
- LED: `calm`
- ポーズ: `neutral`
- サーボ: 安全な中立位置

---

### AUDIO 系

WAV 音声の再生制御。

#### AUDIO:STATUS

現在の音声状態を返す。

```text
AUDIO:STATUS
OK AUDIO STATUS state=Idle playing=false volume=180 size=0 received=0
```

**フィールド**

| フィールド | 説明 |
|---|---|
| `state` | `Idle`, `Receiving`, `Playing`, `Finished`, `Error` のいずれか |
| `playing` | 再生中か |
| `volume` | 現在の音量（0-255） |
| `size` | 受信予定バイト数 |
| `received` | 受信済みバイト数 |

#### AUDIO:VOLUME

音量を設定する。

```text
AUDIO:VOLUME:180
OK AUDIO VOLUME 180
```

音量範囲: `0` 〜 `255`

#### AUDIO:STOP

再生を停止し、`speaking` 状態を解除する。

```text
AUDIO:STOP
OK AUDIO STOP
```

#### AUDIO:WAV

WAV バイナリ転送を開始する。

```text
AUDIO:WAV:245760
READY AUDIO WAV size=245760
<245760 バイトの WAV バイナリ>
OK AUDIO PLAY size=245760
```

転送シーケンス：

1. `AUDIO:WAV:<size>` を送信
2. Firmware が `READY AUDIO WAV` を返す
3. 指定バイト数の WAV バイナリを送信
4. Firmware が `OK AUDIO PLAY` を返す（再生開始）

**対応 WAV 形式**

- PCM / mono / 16-bit
- 任意の有効サンプルレート（16000Hz、24000Hz、44100Hz 等）
- 最大 1MB

**エラー**

```text
ERR AUDIO_TOO_LARGE max=1048576
ERR AUDIO_BUSY
ERR AUDIO_RECEIVE_TIMEOUT
ERR AUDIO_INVALID_FORMAT
ERR AUDIO_TRANSFER_FAILED
```

---

### EVENTS 系

入力イベントの取得・クリア。イベント種類の詳細は [body-features.md](body-features.md) を参照。

#### EVENTS

現在のイベントキューを返す。読み取りだけでキューはクリアされません。

**イベントなし**

```text
EVENTS
OK EVENTS count=0
```

**イベントあり**

```text
EVENTS
OK EVENTS count=2
EVENT id=1 type=touch target=screen value=tap timestamp=12345678 x=123 y=87
EVENT id=2 type=imu target=device value=shake timestamp=12345900
END EVENTS
```

各イベントのフィールド：

| フィールド | 説明 |
|---|---|
| `id` | ブートからの通し番号 |
| `type` | `touch`, `button`, `imu` |
| `target` | `screen`, `button_a`, `button_b`, `button_c`, `device` |
| `value` | `tap`, `pressed`, `moved`, `shake` |
| `timestamp` | `millis()` タイムスタンプ |
| `x`, `y` | タッチ座標（touch イベントのみ） |

#### EVENTS:LATEST

最新のイベント 1 件のみを返す。

```text
EVENTS:LATEST
OK EVENTS LATEST none
```

```text
EVENTS:LATEST
OK EVENTS LATEST id=2 type=imu target=device value=shake timestamp=12345900
```

#### EVENTS:CLEAR

イベントキューをクリアする。

```text
EVENTS:CLEAR
OK EVENTS CLEAR
```

---

### WIFI 系

USB Serial 経由の Wi-Fi 設定管理。通常は Bridge のセットアップ CLI を使用します。手動での操作手順は [wi-fi-setup.md](wi-fi-setup.md) を参照。

#### WIFI:STATUS

```text
WIFI:STATUS
OK WIFI STATUS connected=true ssid=home-wifi ip=192.168.0.123 hostname=stackchan-001 rssi=-55 auth=true
```

#### WIFI:SET_JSON

JSON 形式の Wi-Fi 設定を転送する。通常ユーザは手動送信せず、Bridge CLI を使用してください。

転送シーケンス：

1. `WIFI:SET_JSON:<size>` を送信
2. Firmware が `READY WIFI JSON` を返す
3. 指定バイト数の JSON を送信

```text
WIFI:SET_JSON:85
READY WIFI JSON
<85 bytes of JSON>
OK WIFI CONFIG
```

JSON フィールド：

| フィールド | 必須 | 説明 |
|---|---|---|
| `ssid` | 必須 | Wi-Fi SSID |
| `password` | 必須 | Wi-Fi パスワード |
| `hostname` | 任意 | mDNS ホスト名 |
| `authToken` | 任意 | デバイス HTTP API の認証トークン。未設定時は認証なし |

**エラー**

```text
ERR WIFI_JSON_INVALID
ERR WIFI_CONFIG_SAVE_FAILED
ERR WIFI_CONNECT_FAILED
ERR WIFI_JSON_RECEIVE_TIMEOUT
```

#### WIFI:CONNECT

保存済みの Wi-Fi 設定で接続する。

```text
WIFI:CONNECT
OK WIFI CONNECTED ip=192.168.0.123 hostname=stackchan-001
```

#### WIFI:CLEAR

保存済みの Wi-Fi 設定を消去する。

```text
WIFI:CLEAR
OK WIFI CLEAR
```

---

### DEVICE 系

Wi-Fi 設定とは別の、実行時デバイス設定の管理。

#### DEVICE:CONFIG_JSON

デバイス実行時設定を JSON で転送する。

転送シーケンス：

1. `DEVICE:CONFIG_JSON:<size>` を送信
2. Firmware が `READY DEVICE CONFIG JSON` を返す
3. 指定バイト数の JSON を送信

```text
DEVICE:CONFIG_JSON:42
READY DEVICE CONFIG JSON
<42 bytes of JSON>
OK DEVICE CONFIG
```

JSON フィールド：

| フィールド | 説明 |
|---|---|
| `speechServicesUrl` | 音声サービス（voice-gateway）のエンドポイント URL |
| `wakeAutoStart` | 起動後に Wake Word 待受を自動開始するか |

**エラー**

```text
ERR DEVICE_CONFIG_JSON_INVALID
ERR DEVICE_CONFIG_SAVE_FAILED
ERR DEVICE_CONFIG_JSON_RECEIVE_TIMEOUT
```

---

## エラーコード一覧

| エラーコード | 説明 |
|---|---|
| `UNKNOWN_COMMAND` | 未定義のコマンド |
| `MISSING_ARGUMENT` | 必須引数が不足 |
| `INVALID_ARGUMENT` | 引数の値が許可範囲外 |
| `TOO_MANY_ARGUMENTS` | 引数が多すぎる |
| `COMMAND_TOO_LONG` | コマンドが 128 文字を超過 |
| `AUDIO_TOO_LARGE` | WAV が 1MB を超過 |
| `AUDIO_BUSY` | 音声再生中のため新規受付不可 |
| `AUDIO_RECEIVE_TIMEOUT` | WAV バイナリ受信がタイムアウト |
| `AUDIO_INVALID_FORMAT` | WAV フォーマットが非対応 |
| `AUDIO_TRANSFER_FAILED` | WAV 転送に失敗 |
| `EVENT_QUEUE_ERROR` | イベントキュー内部エラー |
| `UNSUPPORTED_INPUT` | 未対応の入力 |
| `WIFI_JSON_INVALID` | Wi-Fi 設定 JSON が不正 |
| `WIFI_CONFIG_SAVE_FAILED` | Wi-Fi 設定の保存に失敗 |
| `WIFI_CONNECT_FAILED` | Wi-Fi 接続に失敗 |
| `WIFI_JSON_RECEIVE_TIMEOUT` | Wi-Fi 設定 JSON 受信がタイムアウト |
| `DEVICE_CONFIG_JSON_INVALID` | デバイス設定 JSON が不正 |
| `DEVICE_CONFIG_SAVE_FAILED` | デバイス設定の保存に失敗 |
| `DEVICE_CONFIG_JSON_RECEIVE_TIMEOUT` | デバイス設定 JSON 受信がタイムアウト |
| `INTERNAL_ERROR` | 予期しない内部エラー |

---

## 手動テスト例

シリアルモニタで直接入力して確認できます。

```text
PING
VERSION
STATUS
FACE:happy
FACE:doubt
FACE:thinking
LED:calm
POSE:neutral
MOVE:0:0
MOVE:999:999
RESET
HELP
AUDIO:STATUS
AUDIO:VOLUME:180
AUDIO:STOP
EVENTS
EVENTS:LATEST
EVENTS:CLEAR
WIFI:STATUS
WIFI:CONNECT
WIFI:CLEAR
```
