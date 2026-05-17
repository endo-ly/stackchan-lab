# StackChan Bridge

## Overview

StackChan Bridge is a local HTTP API server for controlling StackChan Body Firmware.

It translates HTTP JSON requests into StackChan Serial Protocol v0 commands.

```text
HTTP client
  -> stackchan-bridge
  -> USB Serial
  -> StackChan Body Firmware
```

The bridge does not handle conversation state, personality, memory, TTS, or STT. It is a control bridge between HTTP and the body firmware.

## Requirements

- Node.js
- npm
- USB Serial connection to StackChan
- StackChan Body Firmware Phase 7 or later

## Setup

Install dependencies.

```bash
npm install
```

Build the TypeScript sources.

```bash
npm run build
```

## Configuration

Copy the example config.

```bash
cp config.example.yaml config.yaml
```

Default real-device configuration:

```yaml
serial:
  mode: "real"
  port: "/dev/stackchan"
  baud_rate: 115200
```

`config.yaml` is ignored by git because it is local machine configuration.

## Run

Development mode:

```bash
npm run dev
```

With an explicit config path:

```bash
npm run dev -- config.yaml
```

Production-style run:

```bash
npm run build
npm start
```

By default, the server binds to `127.0.0.1:8787`.

## Mock Mode

Mock mode lets the bridge run without a physical StackChan.

Edit `config.yaml`:

```yaml
serial:
  mode: "mock"
```

Then start the bridge.

```bash
npm run dev -- config.yaml
```

## Smoke Test

With the bridge running:

```bash
npm run smoke
```

To target another URL:

```bash
STACKCHAN_BRIDGE_URL=http://127.0.0.1:8787 npm run smoke
```

The smoke test checks:

- `GET /health`
- `GET /version`
- `GET /capabilities`
- `GET /status`
- `GET /audio/status`
- `GET /events`
- `GET /events/latest`
- `POST /face`
- `POST /led`
- `POST /pose`
- `POST /move`
- `POST /audio/volume`
- `GET /presets`
- `POST /preset`
- `POST /play-wav`
- `POST /audio/stop`
- `POST /events/clear`
- `GET /events`
- `POST /reset`
- invalid `/face` values return HTTP 400

## HTTP API

### GET /health

Bridge process health.

```bash
curl http://127.0.0.1:8787/health
```

### GET /version

Bridge and firmware version.

```bash
curl http://127.0.0.1:8787/version
```

### GET /capabilities

Supported command values.

```bash
curl http://127.0.0.1:8787/capabilities
```

### GET /status

Current StackChan body state.

```bash
curl http://127.0.0.1:8787/status
```

### POST /face

Change face expression.

```bash
curl -X POST http://127.0.0.1:8787/face \
  -H "Content-Type: application/json" \
  -d '{"expression":"doubt"}'
```

Supported expressions:

- `neutral`
- `happy`
- `sad`
- `angry`
- `sleepy`
- `doubt`

`thinking`, `alert`, `listening`, and `speaking` are body presets, not FACE expressions.

### POST /led

Change LED mood.

```bash
curl -X POST http://127.0.0.1:8787/led \
  -H "Content-Type: application/json" \
  -d '{"mood":"calm"}'
```

Supported moods:

- `calm`
- `active`
- `speaking`
- `warning`
- `off`

### POST /pose

Move to a predefined pose.

```bash
curl -X POST http://127.0.0.1:8787/pose \
  -H "Content-Type: application/json" \
  -d '{"pose":"look_left"}'
```

Supported poses:

- `neutral`
- `look_left`
- `look_right`
- `look_up`
- `look_down`

### POST /move

Move servos using explicit x/y values. Values are clamped by bridge safety settings before they are sent to firmware.

```bash
curl -X POST http://127.0.0.1:8787/move \
  -H "Content-Type: application/json" \
  -d '{"x":0,"y":450}'
```

### POST /reset

Return the body to a safe neutral state. This does not reboot the device.

```bash
curl -X POST http://127.0.0.1:8787/reset \
  -H "Content-Type: application/json" \
  -d '{}'
```

### GET /audio/status

Current audio playback state.

```bash
curl http://127.0.0.1:8787/audio/status
```

### POST /audio/volume

Set speaker volume.

```bash
curl -X POST http://127.0.0.1:8787/audio/volume \
  -H "Content-Type: application/json" \
  -d '{"volume":180}'
```

### POST /audio/stop

Stop audio playback.

```bash
curl -X POST http://127.0.0.1:8787/audio/stop
```

### POST /play-wav

Send a WAV file to StackChan and play it.

Supported WAV shape:

- PCM
- mono
- 16-bit
- 16000Hz or 24000Hz
- up to 1MB

```bash
curl -X POST http://127.0.0.1:8787/play-wav \
  -F "file=@test-assets/sample.wav"
```

### GET /events

Get the current firmware-side input event queue. Reading events does not clear them.

```bash
curl http://127.0.0.1:8787/events
```

### GET /events/latest

Get the latest input event, or `null` when no event exists.

```bash
curl http://127.0.0.1:8787/events/latest
```

### POST /events/clear

Clear the firmware-side input event queue.

```bash
curl -X POST http://127.0.0.1:8787/events/clear
```

### GET /presets

List body presets.

```bash
curl http://127.0.0.1:8787/presets
```

### POST /preset

Apply a body preset. Presets are expanded by the bridge into FACE, LED, and POSE commands.

```bash
curl -X POST http://127.0.0.1:8787/preset \
  -H "Content-Type: application/json" \
  -d '{"name":"thinking"}'
```

Supported presets:

- `idle`
- `thinking`
- `alert`
- `listening`
- `speaking`
- `happy`
- `sleepy`
- `error`

## Phase 7 Scope

Implemented:

- `GET /events`
- `GET /events/latest`
- `POST /events/clear`
- Event response parser
- Mock event support
- `POST /play-wav`
- `GET /audio/status`
- `POST /audio/volume`
- `POST /audio/stop`
- `GET /presets`
- `POST /preset`
- Mock support for audio and presets

Not implemented:

- STT
- Wake Word
- TTS generation
- WebSocket
- Server-Sent Events
- External application integration

## Error Shape

Success:

```json
{
  "ok": true,
  "data": {}
}
```

Failure:

```json
{
  "ok": false,
  "error": {
    "code": "INVALID_ARGUMENT",
    "message": "Unsupported expression: thinking"
  }
}
```

## Out of Scope

- TTS
- STT
- WebSocket
- Server-Sent Events
- Authentication
- HTTPS
- Tailscale Serve
- systemd service
- External application integration
- Conversation state
- Long-term memory
- Camera or microphone integration
