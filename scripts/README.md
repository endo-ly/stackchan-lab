# StackChan サービス管理スクリプト

スタックチャンを動かすための Bridge と Voice Gateway を、tmux 上で一括起動・停止するスクリプト群。
ping watchdog により、スタックチャンの電源 ON/OFF に連動して自動でサービスの起動・停止を行う。

## 前提

- Bridge: `bridge/` にビルド済み（`npm run build` 済み）
- Voice Gateway: `/root/workspace/voice-gateway` に依存解決済み（`uv sync` 済み）
- `bridge/config.yaml` にスタックチャンと Voice Gateway の接続先が設定済み

IP アドレス等はすべて `bridge/config.yaml` から読み込む。スクリプト内にハードコードしていない。

## ファイル

| ファイル | 役割 |
|---|---|
| `config.sh` | `bridge/config.yaml` から値を抽出する共有関数 |
| `stackcan-up.sh` | Voice Gateway → Bridge の順で tmux セッションを起動 |
| `stackcan-down.sh` | tmux セッションを停止 |
| `stackcan-watchdog.sh` | スタックチャンの ping 監視と自動起動/停止 |
| `stackcan-watchdog.service` | watchdog を systemd サービスとして常駐させるユニットファイル |

## 使い方

### 手動起動・停止

```bash
# 起動
./scripts/stackcan-up.sh

# 停止
./scripts/stackcan-down.sh

# ログ確認
tmux attach -t stackcan
```

### 自動起動（systemd）

マシン起動時に watchdog を常駐させ、スタックチャンの電源に連動させる。

```bash
sudo cp scripts/stackcan-watchdog.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now stackcan-watchdog
```

状態確認:

```bash
sudo systemctl status stackcan-watchdog
sudo journalctl -u stackcan-watchdog -f
```

## 起動順

Voice Gateway は AivisSpeech Engine の初期化に約 8 秒かかるため、先に起動する。
Bridge は Node.js の起動が約 1 秒と速いため、後に起動する。
これにより初回の STT コールバック失敗を防ぐ。

## 設定

IP アドレスやポートは `bridge/config.yaml` を編集する。スクリプト側に設定値はない。

| 参照元 | 対象 |
|---|---|
| `wifi.base_url` | スタックチャンの IP（watchdog の監視対象） |
| `voice_gateway.base_url` | Voice Gateway のホストとポート |
