import { BridgeError } from "./BridgeError.js";
import { BridgeState } from "./BridgeState.js";
import { bodyPresets } from "./presets.js";
import { clampMove } from "./safety.js";
import { validateWav } from "./wav.js";
import type { BridgeConfig } from "../config/types.js";
import type { SerialProtocolClient } from "../serial/SerialProtocolClient.js";
import { commands, expressions, moods, poses, presets, type Expression, type Mood, type Pose, type PresetName } from "../types/body.js";

export class StackChanBridge {
  private readonly state = new BridgeState();

  constructor(
    private readonly config: BridgeConfig,
    private readonly protocol: SerialProtocolClient,
  ) {}

  async start(): Promise<void> {
    try {
      await this.protocol.connect();
    } catch (error) {
      this.state.markDisconnected(error instanceof Error ? error.message : String(error));
      if (this.config.serial.mode === "real") {
        return;
      }
      throw error;
    }
  }

  async stop(): Promise<void> {
    await this.protocol.disconnect();
  }

  getHealth() {
    return {
      service: "stackchan-bridge",
      status: "ok",
    };
  }

  async getVersion() {
    try {
      this.state.markCommand();
      const device = await this.protocol.version();
      this.state.updateVersion(device);
      this.state.markResponse();
    } catch (error) {
      this.markDeviceError(error);
    }

    return {
      bridge: {
        version: "0.7.0",
      },
      device: this.state.getVersion(),
    };
  }

  async getStatus() {
    try {
      this.state.markCommand();
      const status = await this.protocol.status();
      this.state.updateStatus(status);
      this.state.markResponse();
    } catch (error) {
      this.markDeviceError(error);
    }
    return this.state.getStatus();
  }

  getCapabilities() {
    return {
      expressions,
      moods,
      poses,
      commands,
    };
  }

  getPresets() {
    return { presets };
  }

  async setFace(expression: string) {
    assertAllowed(expression, expressions, "expression");
    await this.runDeviceCommand(() => this.protocol.setFace(expression as Expression));
    this.state.setExpression(titleCase(expression));
    return { expression };
  }

  async setLed(mood: string) {
    assertAllowed(mood, moods, "mood");
    await this.runDeviceCommand(() => this.protocol.setLed(mood as Mood));
    this.state.setMood(titleCase(mood));
    return { mood };
  }

  async setPose(pose: string) {
    assertAllowed(pose, poses, "pose");
    await this.runDeviceCommand(() => this.protocol.setPose(pose as Pose));
    this.state.setPose(titleCase(pose));
    return { pose };
  }

  async move(x: number, y: number) {
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      throw new BridgeError("INVALID_ARGUMENT", "x and y must be finite numbers");
    }
    const safeMove = clampMove({ x, y }, this.config.safety);
    await this.runDeviceCommand(() => this.protocol.move(safeMove));
    this.state.setMove(safeMove.x, safeMove.y);
    return safeMove;
  }

  async reset() {
    await this.runDeviceCommand(() => this.protocol.reset());
    await this.getStatus();
    return { reset: true };
  }

  async getAudioStatus() {
    return await this.runDeviceCommand(() => this.protocol.audioStatus());
  }

  async setAudioVolume(volume: number) {
    if (!Number.isInteger(volume) || volume < 0 || volume > 255) {
      throw new BridgeError("INVALID_ARGUMENT", "volume must be an integer between 0 and 255");
    }
    await this.runDeviceCommand(() => this.protocol.setAudioVolume(volume));
    return { volume };
  }

  async stopAudio() {
    await this.runDeviceCommand(() => this.protocol.stopAudio());
    return { stopped: true };
  }

  async playWav(data: Buffer) {
    validateWav(data);
    await this.runDeviceCommand(() => this.protocol.playWav(data));
    return { playing: true, size: data.length };
  }

  async getEvents() {
    const result = await this.runDeviceCommand(() => this.protocol.events());
    this.state.updateEvents(result.count, result.events);
    return result;
  }

  async getLatestEvent() {
    const event = await this.runDeviceCommand(() => this.protocol.latestEvent());
    this.state.updateLatestEvent(event);
    return { event };
  }

  async clearEvents() {
    await this.runDeviceCommand(() => this.protocol.clearEvents());
    this.state.clearEvents();
    return { cleared: true };
  }

  async applyPreset(name: string) {
    assertAllowed(name, presets, "preset");
    const preset = bodyPresets[name as PresetName];
    try {
      await this.setFace(preset.expression);
      await this.setLed(preset.mood);
      await this.setPose(preset.pose);
      return {
        preset: name,
        applied: preset,
      };
    } catch (error) {
      throw new BridgeError("PRESET_APPLY_FAILED", `Failed to apply preset: ${name}`, error);
    }
  }

  private async runDeviceCommand<T>(command: () => Promise<T>): Promise<T> {
    try {
      this.state.markCommand();
      const result = await command();
      this.state.markResponse();
      return result;
    } catch (error) {
      this.markDeviceError(error);
      throw error;
    }
  }

  private markDeviceError(error: unknown): void {
    this.state.markDisconnected(error instanceof Error ? error.message : String(error));
  }
}

function assertAllowed<T extends readonly string[]>(value: string, allowed: T, field: string): void {
  if (!allowed.includes(value)) {
    throw new BridgeError("INVALID_ARGUMENT", `Unsupported ${field}: ${value}`, { allowed });
  }
}

function titleCase(value: string): string {
  return value
    .split("_")
    .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
    .join("_");
}
