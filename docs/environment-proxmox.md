# Proxmox LXC 環境のセットアップ

Proxmox LXC コンテナ内で Firmware をビルド・書き込みするための追加設定です。Bridge 単体の実行には不要です（Firmware 書き込みにのみ USB パススルーが必要）。

## ホスト側: udev ルール

StackChan (ESP32-S3) 用の固定シンボリックリンクを作成します。

```bash
# /etc/udev/rules.d/99-stackchan.rules
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1001", MODE="0666", SYMLINK+="stackchan"
```

```bash
udevadm control --reload-rules
udevadm trigger
```

USB を抜き差しして確認:

```bash
ls -l /dev/stackchan
# lrwxrwxrwx ... /dev/stackchan -> ttyACM0
```

## ホスト側: LXC デバイスパススルー

コンテナ ID が `100` の場合:

```bash
pct set 100 --dev0 path=/dev/stackchan,mode=0666
pct shutdown 100
pct start 100
```

## コンテナ内: 確認

```bash
ls -l /dev/stackchan
# crw-rw-rw- ... /dev/stackchan

platformio device list
# /dev/stackchan が表示されれば OK
```

## platformio.ini の設定

このプロジェクトの `platformio.ini` はデフォルトで `/dev/stackchan` を使用します。

```ini
monitor_port = /dev/stackchan
upload_port = /dev/stackchan
board_upload.before_reset = usb_reset
```

`board_upload.before_reset = usb_reset` は ESP32-S3 の USB Serial/JTAG に必須です。これがないと書き込み時に `Wrong boot mode detected` エラーが出ます。

## ビルドと書き込み

```bash
cd firmware/body
pio run
pio run -t upload
```

## シリアルモニタ

```bash
pio device monitor -p /dev/stackchan -b 115200
```

## 注意点

- `lxc.mount.entry` による直接バインドマウントは、USB 再接続時に古いデバイスノードを掴み続ける問題があるため非推奨です。Proxmox のデバイスパススルー（`pct set --dev0`）を使用してください。
- `/dev/ttyACM0` は順序依存のため、他の USB シリアル機器の接続順で変化します。必ずシンボリックリンク `/dev/stackchan` を使うようにしてください。
- udev ルールの `MODE="0666"` はシングルユーザ開発環境用です。共有ホストではより狭い権限を推奨します。
