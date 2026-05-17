# Serial Protocol v0

## Overview

StackChan Body FirmwareをUSBシリアル経由で制御するためのテキストプロトコル。

このプロトコルは、PCやミニPCからStackChanの身体制御を行うためのもの。
意味理解、会話状態管理、HTTP API、Wi-Fi接続は扱わない。

## Transport

- USB Serial
- Baud rate: 115200
- Line based
- Newline: `\n` or `\r\n`
- Encoding: ASCII-compatible text
- Maximum command length: 128 characters

## Format

Commands are one line.

```text
COMMAND
COMMAND:arg1
COMMAND:arg1:arg2
```

Command names are case-insensitive.

```text
FACE:happy
face:happy
Face:happy
```

Responses usually start with `OK`, `READY`, `ERR`, or `WARN`.
`EVENTS` returns a multi-line response ending with `END EVENTS`.
Debug logs start with `[` and should not be treated as command responses.

## Commands

### PING

Checks serial connectivity.

```text
PING
OK PONG
```

### VERSION

Returns firmware, protocol, and board versions.

```text
VERSION
OK VERSION firmware=0.7.0 protocol=0.1.0 board=stackchan-cores3
```

### HELP

Returns the supported command list and supported FACE/LED/POSE values.

```text
HELP
OK HELP commands=PING,VERSION,HELP,STATUS,FACE,LED,POSE,MOVE,RESET,AUDIO,EVENTS expressions=neutral,happy,sad,angry,sleepy,doubt moods=calm,active,speaking,warning,off poses=neutral,look_left,look_right,look_up,look_down audio=AUDIO:STATUS,AUDIO:VOLUME,AUDIO:STOP,AUDIO:WAV events=EVENTS,EVENTS:LATEST,EVENTS:CLEAR
```

### STATUS

Returns the current body state.

```text
STATUS
OK STATUS mode=Ready expression=Neutral mood=Calm pose=Neutral x=0 y=0 gazeX=0 gazeY=0 speaking=false blinking=false eventCount=0 latestEvent=none latestTarget=none latestValue=none touchActive=false buttonActive=false imuMoving=false
```

Fields:

- `mode`
- `expression`
- `mood`
- `pose`
- `x`
- `y`
- `gazeX`
- `gazeY`
- `speaking`
- `blinking`
- `eventCount`
- `latestEvent`
- `latestTarget`
- `latestValue`
- `touchActive`
- `buttonActive`
- `imuMoving`

### FACE

Changes the face expression. `FACE` is limited to the face itself; combined body states such as thinking or alert are expanded by the Bridge preset API, not by firmware `FACE`.

```text
FACE:happy
OK FACE happy
```

Supported values:

- `neutral`
- `happy`
- `sad`
- `angry`
- `sleepy`
- `doubt`

Not supported by `FACE`:

- `thinking`
- `alert`

Errors:

```text
ERR MISSING_ARGUMENT expression
ERR INVALID_ARGUMENT expression=thinking
ERR TOO_MANY_ARGUMENTS command=FACE
```

### LED

Changes the LED mood.

```text
LED:calm
OK LED calm
```

Supported values:

- `calm`
- `active`
- `speaking`
- `warning`
- `off`

Errors:

```text
ERR MISSING_ARGUMENT mood
ERR INVALID_ARGUMENT mood=blue
ERR TOO_MANY_ARGUMENTS command=LED
```

### POSE

Moves to a predefined safe pose.

```text
POSE:look_left
OK POSE look_left
```

Supported values:

- `neutral`
- `look_left`
- `look_right`
- `look_up`
- `look_down`

Errors:

```text
ERR MISSING_ARGUMENT pose
ERR INVALID_ARGUMENT pose=spin
ERR TOO_MANY_ARGUMENTS command=POSE
```

### MOVE

Moves the servos using firmware-side safety clamps.

```text
MOVE:0:0
OK MOVE x=0 y=0
```

`MOVE` values are passed to StackChan-BSP as deci-degrees: `10 = 1 degree`.
The current firmware allows:

- `x`: `-450` to `450`
- `y`: `0` to `450`

If requested values are outside the safe range, firmware clamps them and marks the response.

```text
MOVE:999:999
OK MOVE x=450 y=450 clamped=true
```

Errors:

```text
ERR MISSING_ARGUMENT x,y
ERR INVALID_ARGUMENT x=abc
ERR INVALID_ARGUMENT y=abc
ERR TOO_MANY_ARGUMENTS command=MOVE
```

### RESET

Returns the body to a safe neutral state. This does not reboot the ESP32.

```text
RESET
OK RESET
```

Reset applies:

- expression: `neutral`
- mood: `calm`
- pose: `neutral`
- servo: safe neutral position

### AUDIO

Controls WAV audio playback.

#### AUDIO:STATUS

```text
AUDIO:STATUS
OK AUDIO STATUS state=Idle playing=false volume=180 size=0 received=0
```

Fields:

- `state`: `Idle`, `Receiving`, `Playing`, `Finished`, or `Error`
- `playing`
- `volume`
- `size`
- `received`

#### AUDIO:VOLUME

```text
AUDIO:VOLUME:180
OK AUDIO VOLUME 180
```

Volume range is `0` to `255`.

#### AUDIO:STOP

```text
AUDIO:STOP
OK AUDIO STOP
```

Stops playback and clears `speaking`.

#### AUDIO:WAV

Starts a binary WAV transfer.

```text
AUDIO:WAV:245760
READY AUDIO WAV size=245760
<245760 bytes of WAV binary>
OK AUDIO PLAY size=245760
```

Supported WAV shape:

- PCM
- mono
- 16-bit
- 16000Hz or 24000Hz
- max 1MB

Errors:

```text
ERR AUDIO_TOO_LARGE max=1048576
ERR AUDIO_BUSY
ERR AUDIO_RECEIVE_TIMEOUT
ERR AUDIO_INVALID_FORMAT
ERR AUDIO_TRANSFER_FAILED
```

### EVENTS

Returns the current input event queue. Reading events does not clear the queue.

No events:

```text
EVENTS
OK EVENTS count=0
```

With events:

```text
EVENTS
OK EVENTS count=2
EVENT id=1 type=touch target=screen value=tap timestamp=12345678 x=123 y=87
EVENT id=2 type=imu target=device value=shake timestamp=12345900
END EVENTS
```

Each event has:

- `id`: boot-local event sequence number
- `type`: `touch`, `button`, or `imu`
- `target`: `screen`, `button_a`, `button_b`, `button_c`, or `device`
- `value`: `tap`, `pressed`, `moved`, or `shake`
- `timestamp`: `millis()` timestamp
- `x`, `y`: touch coordinates. Present only for touch events.

### EVENTS:LATEST

Returns only the latest input event.

```text
EVENTS:LATEST
OK EVENTS LATEST none
```

```text
EVENTS:LATEST
OK EVENTS LATEST id=2 type=imu target=device value=shake timestamp=12345900
```

### EVENTS:CLEAR

Clears the input event queue.

```text
EVENTS:CLEAR
OK EVENTS CLEAR
```

## Responses

Successful responses:

```text
OK PONG
OK FACE happy
OK LED calm
OK POSE neutral
OK MOVE x=0 y=0
OK AUDIO STATUS state=Idle playing=false volume=180 size=0 received=0
OK AUDIO VOLUME 180
OK AUDIO STOP
OK AUDIO PLAY size=245760
OK EVENTS count=0
OK EVENTS LATEST none
OK EVENTS CLEAR
```

Error responses:

```text
ERR UNKNOWN_COMMAND command=FOO
ERR MISSING_ARGUMENT expression
ERR INVALID_ARGUMENT expression=thinking
ERR TOO_MANY_ARGUMENTS command=MOVE
ERR COMMAND_TOO_LONG max=128
ERR AUDIO_TOO_LARGE max=1048576
ERR AUDIO_BUSY
ERR AUDIO_RECEIVE_TIMEOUT
ERR AUDIO_INVALID_FORMAT
ERR AUDIO_TRANSFER_FAILED
ERR EVENT_QUEUE_ERROR
ERR UNSUPPORTED_INPUT
ERR INTERNAL_ERROR
```

Warning responses are reserved for later use. Servo clamping is reported as `clamped=true` in the final `OK MOVE` response.

## Error Codes

- `UNKNOWN_COMMAND`
- `MISSING_ARGUMENT`
- `INVALID_ARGUMENT`
- `TOO_MANY_ARGUMENTS`
- `COMMAND_TOO_LONG`
- `AUDIO_TOO_LARGE`
- `AUDIO_BUSY`
- `AUDIO_RECEIVE_TIMEOUT`
- `AUDIO_INVALID_FORMAT`
- `AUDIO_TRANSFER_FAILED`
- `EVENT_QUEUE_ERROR`
- `UNSUPPORTED_INPUT`
- `INTERNAL_ERROR`

## Manual Examples

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
```
