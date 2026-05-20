import type { BridgeInputEvent, LatestTranscription, TranscriptionResult } from "../stt/SttTypes.js";
import type { BodyStatus, DeviceVersion, InputEvent, ReceivedInputEvent } from "../types/body.js";

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
  private eventCount?: number;
  private latestEvent?: ReceivedInputEvent;
  private touchActive?: boolean;
  private buttonActive?: boolean;
  private imuMoving?: boolean;
  private lastCommandAt?: string;
  private lastResponseAt?: string;
  private lastError?: string;
  private sttEnabled = false;
  private sttReachable = false;
  private latestTranscription?: LatestTranscription;
  private bridgeEventCount = 0;
  private latestBridgeEvent?: BridgeInputEvent;

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
    this.eventCount = status.eventCount ?? this.eventCount;
    this.touchActive = status.touchActive ?? this.touchActive;
    this.buttonActive = status.buttonActive ?? this.buttonActive;
    this.imuMoving = status.imuMoving ?? this.imuMoving;
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

  updateEvents(count: number, events: InputEvent[]): void {
    this.eventCount = count;
    const latest = events.at(-1);
    if (latest) {
      this.latestEvent = withReceivedAt(latest);
    }
  }

  updateLatestEvent(event: InputEvent | null): void {
    if (event) {
      this.eventCount = Math.max(this.eventCount ?? 0, 1);
      this.latestEvent = withReceivedAt(event);
    }
  }

  clearEvents(): void {
    this.eventCount = 0;
    this.latestEvent = undefined;
  }

  updateSttHealth(enabled: boolean, reachable: boolean): void {
    this.sttEnabled = enabled;
    this.sttReachable = reachable;
  }

  updateTranscription(result: TranscriptionResult, timestamp: string): LatestTranscription {
    this.latestTranscription = {
      ...result,
      timestamp,
    };
    return this.latestTranscription;
  }

  getLatestTranscription(): LatestTranscription | null {
    return this.latestTranscription ?? null;
  }

  updateBridgeEvents(events: BridgeInputEvent[]): void {
    this.bridgeEventCount = events.length;
    this.latestBridgeEvent = events.at(-1);
  }

  clearBridgeEvents(): void {
    this.bridgeEventCount = 0;
    this.latestBridgeEvent = undefined;
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

  getStatus(): BodyStatus & {
    lastError?: string;
    sttEnabled: boolean;
    sttReachable: boolean;
    latestTranscription?: LatestTranscription;
    bridgeEventCount: number;
    latestBridgeEvent?: BridgeInputEvent;
  } {
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
      eventCount: this.eventCount,
      latestEvent: this.latestEvent?.type,
      latestTarget: this.latestEvent?.target,
      latestValue: this.latestEvent?.value,
      touchActive: this.touchActive,
      buttonActive: this.buttonActive,
      imuMoving: this.imuMoving,
      lastCommandAt: this.lastCommandAt,
      lastResponseAt: this.lastResponseAt,
      lastError: this.lastError,
      sttEnabled: this.sttEnabled,
      sttReachable: this.sttReachable,
      latestTranscription: this.latestTranscription,
      bridgeEventCount: this.bridgeEventCount,
      latestBridgeEvent: this.latestBridgeEvent,
    };
  }
}

function withReceivedAt(event: InputEvent): ReceivedInputEvent {
  return {
    ...event,
    receivedAt: new Date().toISOString(),
  };
}
