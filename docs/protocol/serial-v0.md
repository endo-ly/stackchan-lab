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

Responses are one line and start with `OK`, `ERR`, or `WARN`.
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
OK VERSION firmware=0.4.0 protocol=0.1.0 board=stackchan-cores3
```

### HELP

Returns the supported command list and supported FACE/LED/POSE values.

```text
HELP
OK HELP commands=PING,VERSION,HELP,STATUS,FACE,LED,POSE,MOVE,RESET expressions=neutral,happy,sad,angry,sleepy,doubt moods=calm,active,speaking,warning,off poses=neutral,look_left,look_right,look_up,look_down
```

### STATUS

Returns the current body state.

```text
STATUS
OK STATUS mode=Ready expression=Neutral mood=Calm pose=Neutral x=0 y=0 gazeX=0 gazeY=0 speaking=false blinking=false
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

### FACE

Changes the face expression. `FACE` is limited to the face itself; combined body states such as thinking or alert belong to a future `PRESET` command.

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

If requested values are outside the safe range, firmware clamps them and marks the response.

```text
MOVE:999:999
OK MOVE x=15 y=10 clamped=true
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

## Responses

Successful responses:

```text
OK PONG
OK FACE happy
OK LED calm
OK POSE neutral
OK MOVE x=0 y=0
```

Error responses:

```text
ERR UNKNOWN_COMMAND command=FOO
ERR MISSING_ARGUMENT expression
ERR INVALID_ARGUMENT expression=thinking
ERR TOO_MANY_ARGUMENTS command=MOVE
ERR COMMAND_TOO_LONG max=128
ERR INTERNAL_ERROR
```

Warning responses are reserved for later use. Servo clamping is reported as `clamped=true` in the final `OK MOVE` response.

## Error Codes

- `UNKNOWN_COMMAND`
- `MISSING_ARGUMENT`
- `INVALID_ARGUMENT`
- `TOO_MANY_ARGUMENTS`
- `COMMAND_TOO_LONG`
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
```
