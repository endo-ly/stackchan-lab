# StackChan Bridge

## Overview

StackChan Bridge is a local HTTP API server for controlling StackChan Body Firmware.

It translates HTTP JSON requests into StackChan device commands.

```text
HTTP client
  -> stackchan-bridge
  -> USB Serial or Wi-Fi HTTP
  -> StackChan Body Firmware
```

The bridge does not handle conversation state, personality, memory, or TTS. It can call an external STT service and store the transcription as a bridge-side input event.

## Requirements

- Node.js
- npm
- USB Serial connection to StackChan for serial mode
- Wi-Fi reachable StackChan for wifi mode
- StackChan Body Firmware Phase 8 or later for wifi mode
- speech-services for STT mode

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
device:
  transport: "serial"

serial:
  mode: "real"
  port: "/dev/stackchan"
  baud_rate: 115200
```

Wi-Fi transport configuration:

```yaml
device:
  transport: "wifi"

wifi:
  base_url: "http://192.168.0.123"
  token: "<device-token>"
  timeout_ms: 5000
```

STT configuration:

```yaml
stt:
  transcribe_url: "http://<speech-services-lan-ip>:8790/transcribe"

events:
  include_bridge_events: true
```

`speech-services` is a separate Python service. Audio sources send audio to `speech-services`; the bridge receives completed STT events.

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

When `speech-services` is running and STT is enabled in `config.yaml`, include a sample WAV:

```bash
STACKCHAN_STT_SAMPLE_WAV=/root/workspace/speech-services/sample/jsut-basic5000-4501-16k.wav npm run smoke
```

## Phase 8 Scope

Implemented:

- DeviceTransport abstraction
- SerialTransport
- WifiTransport
- transport selection by config
- wireless StackChan control
- wireless audio playback
- wireless event retrieval
- device unreachable / timeout / unauthorized error mapping

Config:

- `device.transport = serial | wifi`
- `wifi.base_url`
- `wifi.token`
- `wifi.timeout_ms`

Serial mode:

```text
Bridge HTTP API -> SerialTransport -> USB Serial -> StackChan
```

Wi-Fi mode:

```text
Bridge HTTP API -> WifiTransport -> HTTP over Wi-Fi -> StackChan
```

The external Bridge API stays the same in both modes.

## Wi-Fi Mode Smoke Test

First save Wi-Fi credentials to the device over USB Serial.

Stop any running bridge process before using the setup CLI, because USB Serial can only be opened by one process at a time.

```bash
cp config.example.yaml config.yaml
npm run wifi:setup -- config.yaml
```

`config.yaml` is local machine configuration. The Wi-Fi SSID, password, and device token are not written to this file by the setup CLI.

The CLI prompts for:

- SSID
- Wi-Fi password, hidden while typing
- hostname
- device auth token, hidden while typing

Generate a device auth token locally:

```bash
openssl rand -hex 32
```

The password and token are sent directly to StackChan over USB Serial and are not written to the repository.

Device runtime config is updated separately:

```bash
npm run device:config -- config.yaml
```

`device:config` sends `stt.transcribe_url` to StackChan over USB Serial. This is used by the body microphone recorder.

```yaml
stt:
  transcribe_url: "http://<speech-services-lan-ip>:8790/transcribe"
```

To regenerate the token later:

```bash
openssl rand -hex 32
npm run wifi:setup -- config.yaml
```

Enter the new token when prompted, then update `wifi.token` in local `config.yaml` to the same value.

After setup, configure `config.yaml`:

```yaml
device:
  transport: "wifi"

wifi:
  base_url: "http://192.168.0.123"
  token: "<device-token>"
  timeout_ms: 5000
```

Start the bridge:

```bash
npm run dev -- config.yaml
```

Then run:

```bash
curl http://127.0.0.1:8787/status

curl -X POST http://127.0.0.1:8787/preset \
  -H "Content-Type: application/json" \
  -d '{"name":"thinking"}'

curl http://127.0.0.1:8787/events
```

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

### POST /stt/events

Receive a completed transcription event from `speech-services`. The bridge stores it as the latest STT result and exposes it through `/events`.

```bash
curl -X POST http://127.0.0.1:8787/stt/events \
  -H "Content-Type: application/json" \
  -d '{
    "source": "stackchan",
    "text": "こんにちは",
    "language": "ja",
    "durationSec": 3.2,
    "processingMs": 820,
    "provider": "reazonspeech_k2",
    "model": "reazon-research/reazonspeech-k2-v2",
    "audio": {
      "sampleRate": 16000,
      "channels": 1,
      "format": "wav"
    }
  }'
```

The response includes `processingMs` from `speech-services`.

### GET /stt/latest

Get the latest STT result.

```bash
curl http://127.0.0.1:8787/stt/latest
```

### GET /events

Get the current input event queue. Firmware-side events are returned with `source: "device"`. STT events keep their audio input source such as `source: "stackchan"` when `events.include_bridge_events` is true. Reading events does not clear them.

```bash
curl http://127.0.0.1:8787/events
```

### GET /events/latest

Get the latest input event, or `null` when no event exists.

```bash
curl http://127.0.0.1:8787/events/latest
```

### POST /events/clear

Clear the firmware-side input event queue and bridge-side events.

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

## Phase 9 Scope

Implemented:

- `GET /stt/latest`
- STT result as bridge-side input event
- `/events` integration for bridge-side events

## Phase 9.5 Scope

Implemented:

- removed bridge-side audio upload STT endpoint
- `POST /stt/events`
- latest STT event received from `speech-services`
- `/events` integration for STT input events

Removed:

- `POST /stt/transcribe-file`

Not implemented:

- Wake Word
- Streaming STT
- StackChan microphone capture
- Conversation integration

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
