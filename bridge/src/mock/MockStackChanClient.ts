import type { StackChanClient } from "../serial/StackChanClient.js";

export class MockStackChanClient implements StackChanClient {
  private connected = false;
  private state = {
    mode: "Ready",
    expression: "Neutral",
    mood: "Calm",
    pose: "Neutral",
    x: 0,
    y: 450,
    gazeX: 0,
    gazeY: 0,
    speaking: false,
    blinking: false,
  };
  private audio = {
    state: "Idle",
    playing: false,
    volume: 180,
    size: 0,
    received: 0,
  };
  private pendingWavSize = 0;

  async connect(): Promise<void> {
    this.connected = true;
  }

  async disconnect(): Promise<void> {
    this.connected = false;
  }

  isConnected(): boolean {
    return this.connected;
  }

  async sendCommand(command: string): Promise<string> {
    const [name, ...args] = command.trim().split(":");

    switch (name?.toUpperCase()) {
      case "PING":
        return "OK PONG";
      case "VERSION":
        return "OK VERSION firmware=mock protocol=0.1.0 board=mock-stackchan";
      case "STATUS":
        return this.statusLine();
      case "FACE":
        return this.setFace(args[0]);
      case "LED":
        return this.setLed(args[0]);
      case "POSE":
        return this.setPose(args[0]);
      case "MOVE":
        return this.move(args[0], args[1]);
      case "RESET":
        this.state.expression = "Neutral";
        this.state.mood = "Calm";
        this.state.pose = "Neutral";
        this.state.x = 0;
        this.state.y = 450;
        return "OK RESET";
      case "AUDIO":
        return this.audioCommand(args);
      default:
        return `ERR UNKNOWN_COMMAND command=${name ?? ""}`;
    }
  }

  async sendBinary(data: Buffer): Promise<string> {
    if (this.pendingWavSize <= 0 || data.length !== this.pendingWavSize) {
      return "ERR AUDIO_TRANSFER_FAILED";
    }
    this.audio.state = "Playing";
    this.audio.playing = true;
    this.audio.size = data.length;
    this.audio.received = data.length;
    this.state.speaking = true;
    this.pendingWavSize = 0;
    return `OK AUDIO PLAY size=${data.length}`;
  }

  async sendCommandWithBinary(command: string, data: Buffer): Promise<{ ready: string; final: string }> {
    const ready = await this.sendCommand(command);
    const final = ready.startsWith("READY ") ? await this.sendBinary(data) : ready;
    return { ready, final };
  }

  private statusLine(): string {
    const s = this.state;
    return `OK STATUS mode=${s.mode} expression=${s.expression} mood=${s.mood} pose=${s.pose} x=${s.x} y=${s.y} gazeX=${s.gazeX} gazeY=${s.gazeY} speaking=${s.speaking} blinking=${s.blinking}`;
  }

  private setFace(expression: string | undefined): string {
    if (!expression) {
      return "ERR MISSING_ARGUMENT expression";
    }
    if (!["neutral", "happy", "sad", "angry", "sleepy", "doubt"].includes(expression)) {
      return `ERR INVALID_ARGUMENT expression=${expression}`;
    }
    this.state.expression = titleCase(expression);
    return `OK FACE ${expression}`;
  }

  private setLed(mood: string | undefined): string {
    if (!mood) {
      return "ERR MISSING_ARGUMENT mood";
    }
    if (!["calm", "active", "speaking", "warning", "off"].includes(mood)) {
      return `ERR INVALID_ARGUMENT mood=${mood}`;
    }
    this.state.mood = titleCase(mood);
    return `OK LED ${mood}`;
  }

  private setPose(pose: string | undefined): string {
    if (!pose) {
      return "ERR MISSING_ARGUMENT pose";
    }
    if (!["neutral", "look_left", "look_right", "look_up", "look_down"].includes(pose)) {
      return `ERR INVALID_ARGUMENT pose=${pose}`;
    }
    this.state.pose = titleCase(pose);
    return `OK POSE ${pose}`;
  }

  private move(rawX: string | undefined, rawY: string | undefined): string {
    const x = Number(rawX);
    const y = Number(rawY);
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      return "ERR INVALID_ARGUMENT x,y";
    }
    this.state.x = x;
    this.state.y = y;
    return `OK MOVE x=${x} y=${y}`;
  }

  private audioCommand(args: string[]): string {
    const action = args[0]?.toLowerCase();
    if (action === "status") {
      return `OK AUDIO STATUS state=${this.audio.state} playing=${this.audio.playing} volume=${this.audio.volume} size=${this.audio.size} received=${this.audio.received}`;
    }
    if (action === "volume") {
      const volume = Number(args[1]);
      if (!Number.isInteger(volume) || volume < 0 || volume > 255) {
        return `ERR INVALID_ARGUMENT volume=${args[1] ?? ""}`;
      }
      this.audio.volume = volume;
      return `OK AUDIO VOLUME ${volume}`;
    }
    if (action === "stop") {
      this.audio.state = "Idle";
      this.audio.playing = false;
      this.state.speaking = false;
      return "OK AUDIO STOP";
    }
    if (action === "wav") {
      const size = Number(args[1]);
      if (!Number.isInteger(size) || size <= 0) {
        return `ERR INVALID_ARGUMENT size=${args[1] ?? ""}`;
      }
      if (size > 1048576) {
        return "ERR AUDIO_TOO_LARGE max=1048576";
      }
      if (this.audio.playing) {
        return "ERR AUDIO_BUSY";
      }
      this.pendingWavSize = size;
      this.audio.state = "Receiving";
      return `READY AUDIO WAV size=${size}`;
    }
    return `ERR INVALID_ARGUMENT audio_command=${args[0] ?? ""}`;
  }
}

function titleCase(value: string): string {
  return value
    .split("_")
    .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
    .join("_");
}
