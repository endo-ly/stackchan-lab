# Wi-Fi セットアップ

StackChan を USB ケーブルなしで Wi-Fi 経由制御するための設定手順です。

## 概要

Wi-Fi モードでは、Bridge と StackChan が同一 LAN 内で HTTP 通信します。これにより USB ケーブルが不要になり、設置の自由度が上がります。

```text
Bridge（Linux/Mac）── Wi-Fi LAN ── StackChan（ESP32-S3）
     HTTP                          HTTP Server
     10.0.0.100                    10.0.0.123
```

## 前提条件

| 条件 | 確認方法 |
|---|---|
| **2.4GHz Wi-Fi AP** | ESP32-S3 は 5GHz 非対応です。AP が 2.4GHz 帯を有効にしていることを確認 |
| **Firmware 書き込み済み** | Firmware が動作していること（[firmware/body/README.md](../firmware/body/README.md) 参照） |
| **USB Serial 接続可能** | セットアップは USB Serial 経由で行います。セットアップ後の運用には不要です |
| **Bridge が停止中** | USB Serial は排他利用のため、Bridge プロセスが起動していると CLI が接続できません |

---

## 手順

### 1. デバイス認証トークンの生成

StackChan 本体の HTTP API を保護するためのトークンを作成します。

```bash
openssl rand -hex 32
```

出力例: `a1b2c3d4e5f6...`（64 文字の 16 進数文字列）

> **セキュリティ**: トークン未設定の場合、StackChan の HTTP API は認証なしの開発モードで動作します。同一 LAN 内の他端末からもアクセス可能になるため、本番運用時は必ずトークンを設定してください。

### 2. Bridge の停止確認

USB Serial は 1 プロセスしかオープンできないため、Bridge が起動している場合は停止します。

```bash
pkill -f "tsx src/main.ts"
pkill -f "node dist/main.js"
```

### 3. Wi-Fi 設定 CLI の実行

```bash
cd bridge
cp config.example.yaml config.yaml
npm run wifi:setup -- config.yaml
```

CLI が対話的に以下の情報を尋ねます。

| 入力項目 | 説明 |
|---|---|
| **SSID** | 接続先 Wi-Fi の SSID（2.4GHz） |
| **Password** | Wi-Fi パスワード（入力中は表示されません） |
| **Hostname** | StackChan の mDNS ホスト名（例: `stackchan-001`） |
| **Auth Token** | 手順 1 で生成したトークン（入力中は表示されません） |

CLI は内部的に以下を行います。

1. 入力内容を JSON 化
2. `WIFI:SET_JSON:<size>` コマンドをシリアル送信
3. Firmware が `READY WIFI JSON` を返したら JSON 本体を転送
4. Firmware が ESP32 Preferences（NVS）に設定を保存

> **注意**: SSID とパスワード・トークンはリポジトリ内のファイルに一切保存されません。StackChan 本体の不揮発メモリにのみ保存されます。

### 4. デバイス設定の転送（任意）

STT 連携など、運用に必要な設定を別途送信します。

```bash
npm run device:config -- config.yaml
```

これにより `config.yaml` の `voice_gateway.base_url` から組み立てた `/v1/transcribe` と、`wake.record_duration_ms` が StackChan 本体に送信されます。Wi-Fi 設定とは別のコマンド（`DEVICE:CONFIG_JSON`）で転送されます。

### 5. Bridge の設定変更

`config.yaml` を Wi-Fi モード用に編集します。

```yaml
device:
  transport: "wifi"

wifi:
  base_url: "http://192.168.0.123"  # StackChan の IP アドレス
  token: "<手順1で生成したトークン>"
  timeout_ms: 5000
```

### 6. StackChan の IP アドレス確認

Firmware のシリアルモニタで起動ログを確認します。

```bash
cd firmware/body
pio device monitor -p /dev/stackchan -b 115200
```

起動後、以下のようなログが表示されます。

```text
[NET] Wi-Fi connected
[NET] IP: 192.168.0.123
[NET] hostname: stackchan-001
[HTTP] server started on port 80
```

あるいは、mDNS（Bonjour）で名前解決できる場合：

```bash
ping stackchan-001.local
```

### 7. Bridge 起動と確認

```bash
cd bridge
npm start
```

```bash
# ヘルスチェック
curl http://127.0.0.1:8787/health

# StackChan の状態確認
curl http://127.0.0.1:8787/status

# 表情変更テスト
curl -X POST http://127.0.0.1:8787/face \
  -H "Content-Type: application/json" \
  -d '{"expression":"happy"}'
```

---

## 直接確認（トラブルシュート用）

Bridge を介さず、StackChan 本体の HTTP API を直接叩いて確認できます。

```bash
STACKCHAN_IP=192.168.0.123
TOKEN=<手順1で生成したトークン>

# ヘルスチェック（認証不要）
curl "http://$STACKCHAN_IP/health"

# 状態確認（認証必要）
curl "http://$STACKCHAN_IP/status" \
  -H "X-StackChan-Token: $TOKEN"

# 表情変更
curl -X POST "http://$STACKCHAN_IP/face" \
  -H "Content-Type: application/json" \
  -H "X-StackChan-Token: $TOKEN" \
  -d '{"expression":"happy"}'
```

---

## Wi-Fi 設定の管理

### 設定状態の確認

```bash
# シリアル経由
echo "WIFI:STATUS" | picocom -b 115200 /dev/stackchan
```

```text
OK WIFI STATUS connected=true ssid=my-wifi ip=192.168.0.123 hostname=stackchan-001 rssi=-55 auth=true
```

### 再接続

保存済みの設定で再接続します。

```text
WIFI:CONNECT
OK WIFI CONNECTED ip=192.168.0.123 hostname=stackchan-001
```

### 設定の消去

StackChan 本体から Wi-Fi 設定を削除します。

```text
WIFI:CLEAR
OK WIFI CLEAR
```

---

## トークンの再生成

トークンを変更する場合：

```bash
# 1. 新しいトークンを生成
openssl rand -hex 32

# 2. Wi-Fi 設定を再実行（新しいトークンを入力）
npm run wifi:setup -- config.yaml

# 3. Bridge の config.yaml も更新
# wifi.token を新しいトークンに変更

# 4. Bridge を再起動
```

---

## トラブルシュート

| 症状 | 確認ポイント |
|---|---|
| Wi-Fi に接続できない | SSID が 2.4GHz 帯か確認。5GHz のみの AP は非対応 |
| Bridge から `DEVICE_UNREACHABLE` | StackChan の IP が正しいか。`ping` で到達性を確認 |
| `DEVICE_UNAUTHORIZED` | `config.yaml` の `wifi.token` が StackChan 本体のトークンと一致しているか |
| CLI がシリアルポートを開けない | Bridge プロセスが停止しているか（`pkill` で確認） |
| USB Serial が `/dev/stackchan` に見つからない | シンボリックリンクが切れていないか。`ls -l /dev/stackchan` で確認 |

その他の問題は [README.md](../README.md) のトラブルシュートセクションを参照。
