import type { DeviceTransport, DeviceStatus, DeviceVersionInfo } from "./DeviceTransport.js";
import type { SafeMove } from "../bridge/safety.js";
import type { AudioStatus, Expression, InputEvent, InputEvents, Mood, Pose } from "../types/body.js";
import type { SerialProtocolClient } from "../serial/SerialProtocolClient.js";

export class SerialTransport implements DeviceTransport {
  constructor(private readonly protocol: SerialProtocolClient) {}

  async connect(): Promise<void> {
    await this.protocol.connect();
  }

  async disconnect(): Promise<void> {
    await this.protocol.disconnect();
  }

  isConnected(): boolean {
    return this.protocol.isConnected();
  }

  async version(): Promise<DeviceVersionInfo> {
    return await this.protocol.version();
  }

  async status(): Promise<DeviceStatus> {
    return await this.protocol.status();
  }

  async setFace(expression: Expression): Promise<void> {
    await this.protocol.setFace(expression);
  }

  async setLed(mood: Mood): Promise<void> {
    await this.protocol.setLed(mood);
  }

  async setPose(pose: Pose): Promise<void> {
    await this.protocol.setPose(pose);
  }

  async move(move: Pick<SafeMove, "x" | "y">): Promise<void> {
    await this.protocol.move(move);
  }

  async reset(): Promise<void> {
    await this.protocol.reset();
  }

  async audioStatus(): Promise<AudioStatus> {
    return await this.protocol.audioStatus();
  }

  async setAudioVolume(volume: number): Promise<void> {
    await this.protocol.setAudioVolume(volume);
  }

  async stopAudio(): Promise<void> {
    await this.protocol.stopAudio();
  }

  async playWav(data: Buffer): Promise<void> {
    await this.protocol.playWav(data);
  }

  async events(): Promise<InputEvents> {
    return await this.protocol.events();
  }

  async latestEvent(): Promise<InputEvent | null> {
    return await this.protocol.latestEvent();
  }

  async clearEvents(): Promise<void> {
    await this.protocol.clearEvents();
  }
}
