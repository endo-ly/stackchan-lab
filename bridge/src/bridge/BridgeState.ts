import type { BodyStatus, DeviceVersion } from "../types/body.js";

export class BridgeState {
  private connected = false;
  private firmwareVersion?: string;
  private protocolVersion?: string;
  private board?: string;
  private mode?: string;
  private expression?: string;
  private mood?: string;
  private pose?: string;
  private x?: number;
  private y?: number;
  private gazeX?: number;
  private gazeY?: number;
  private speaking?: boolean;
  private blinking?: boolean;
  private lastCommandAt?: string;
  private lastResponseAt?: string;
  private lastError?: string;

  markCommand(): void {
    this.lastCommandAt = new Date().toISOString();
  }

  markResponse(): void {
    this.connected = true;
    this.lastResponseAt = new Date().toISOString();
    this.lastError = undefined;
  }

  markDisconnected(error?: string): void {
    this.connected = false;
    this.lastError = error;
  }

  updateVersion(version: Omit<DeviceVersion, "connected">): void {
    this.firmwareVersion = version.firmware;
    this.protocolVersion = version.protocol;
    this.board = version.board;
  }

  updateStatus(status: Omit<BodyStatus, "connected" | "lastCommandAt" | "lastResponseAt">): void {
    this.mode = status.mode ?? this.mode;
    this.expression = status.expression ?? this.expression;
    this.mood = status.mood ?? this.mood;
    this.pose = status.pose ?? this.pose;
    this.x = status.x ?? this.x;
    this.y = status.y ?? this.y;
    this.gazeX = status.gazeX ?? this.gazeX;
    this.gazeY = status.gazeY ?? this.gazeY;
    this.speaking = status.speaking ?? this.speaking;
    this.blinking = status.blinking ?? this.blinking;
  }

  setExpression(expression: string): void {
    this.expression = expression;
  }

  setMood(mood: string): void {
    this.mood = mood;
  }

  setPose(pose: string): void {
    this.pose = pose;
  }

  setMove(x: number, y: number): void {
    this.x = x;
    this.y = y;
  }

  getVersion(): DeviceVersion {
    if (!this.connected) {
      return { connected: false };
    }
    return {
      connected: true,
      firmware: this.firmwareVersion,
      protocol: this.protocolVersion,
      board: this.board,
    };
  }

  getStatus(): BodyStatus & { lastError?: string } {
    return {
      connected: this.connected,
      mode: this.mode,
      expression: this.expression,
      mood: this.mood,
      pose: this.pose,
      x: this.x,
      y: this.y,
      gazeX: this.gazeX,
      gazeY: this.gazeY,
      speaking: this.speaking,
      blinking: this.blinking,
      lastCommandAt: this.lastCommandAt,
      lastResponseAt: this.lastResponseAt,
      lastError: this.lastError,
    };
  }
}
