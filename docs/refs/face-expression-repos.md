# Face & Expression References

## m5stack-avatar

- 顔描画
- 表情
- 口パク
- カスタム顔
- 注意点: 依存は `FaceController` 内に閉じ込める

## stackchan-atama

- USBシリアルによるFACE制御
- WAV再生 + 口パク
- 表情一覧
- PC側TTSとの分離

## stack-chan

- cute face
- expression
- glance / stare / gaze
- say things

## 採用判断

- Phase 4ではM5Stack-Avatarを利用する
- `FACE` は顔そのものの表情に限定する
- `thinking` / `alert` は `FACE` ではなく後続の `PRESET` で扱う
- 依存は `FaceController` 内に閉じ込める
- `M5Stack-Avatar` は内部で描画タスクを起動するため、`avatar.init()` 直後に表情変更APIを連続実行しない
- 既存Repoを丸ごとコピーしない
