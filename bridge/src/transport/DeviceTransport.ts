import type { SafeMove } from "../bridge/safety.js";
import type { AudioStatus, BodyStatus, DeviceVersion, Expression, InputEvent, InputEvents, Mood, Pose } from "../types/body.js";

export type DeviceStatus = Omit<BodyStatus, "connected" | "lastCommandAt" | "lastResponseAt">;
export type DeviceVersionInfo = Omit<DeviceVersion, "connected">;

export interface DeviceTransport {
  connect(): Promise<void>;
  disconnect(): Promise<void>;
  isConnected(): boolean;

  version(): Promise<DeviceVersionInfo>;
  status(): Promise<DeviceStatus>;

  setFace(expression: Expression): Promise<void>;
  setLed(mood: Mood): Promise<void>;
  setPose(pose: Pose): Promise<void>;
  move(move: Pick<SafeMove, "x" | "y">): Promise<void>;
  reset(): Promise<void>;

  audioStatus(): Promise<AudioStatus>;
  setAudioVolume(volume: number): Promise<void>;
  stopAudio(): Promise<void>;
  playWav(data: Buffer): Promise<void>;

  events(): Promise<InputEvents>;
  latestEvent(): Promise<InputEvent | null>;
  clearEvents(): Promise<void>;
}
