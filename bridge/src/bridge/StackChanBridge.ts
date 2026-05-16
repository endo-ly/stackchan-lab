import { BridgeError } from "./BridgeError.js";
import { BridgeState } from "./BridgeState.js";
import { clampMove } from "./safety.js";
import type { BridgeConfig } from "../config/types.js";
import type { SerialProtocolClient } from "../serial/SerialProtocolClient.js";
import { commands, expressions, moods, poses, type Expression, type Mood, type Pose } from "../types/body.js";

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
        version: "0.5.0",
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
    this.state.updateStatus({
      expression: "Neutral",
      mood: "Calm",
      pose: "Neutral",
      x: 0,
      y: this.config.safety.maxY,
      gazeX: 0,
      gazeY: 0,
      speaking: false,
      blinking: false,
    });
    return { reset: true };
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
