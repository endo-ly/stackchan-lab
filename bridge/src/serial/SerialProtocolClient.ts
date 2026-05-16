import { BridgeError } from "../bridge/BridgeError.js";
import type { AudioStatus, BodyStatus, DeviceVersion, Expression, Mood, Pose } from "../types/body.js";
import type { SafeMove } from "../bridge/safety.js";
import type { StackChanClient } from "./StackChanClient.js";
import { parseSerialResponse } from "./parseSerialResponse.js";

export class SerialProtocolClient {
  constructor(private readonly client: StackChanClient) {}

  async connect(): Promise<void> {
    await this.client.connect();
  }

  async disconnect(): Promise<void> {
    await this.client.disconnect();
  }

  isConnected(): boolean {
    return this.client.isConnected();
  }

  async version(): Promise<Omit<DeviceVersion, "connected">> {
    const response = await this.send("VERSION");
    expectCommand(response.command, "VERSION", response.raw);
    return {
      firmware: response.fields.firmware,
      protocol: response.fields.protocol,
      board: response.fields.board,
    };
  }

  async status(): Promise<Omit<BodyStatus, "connected" | "lastCommandAt" | "lastResponseAt">> {
    const response = await this.send("STATUS");
    expectCommand(response.command, "STATUS", response.raw);
    return {
      mode: response.fields.mode,
      expression: response.fields.expression,
      mood: response.fields.mood,
      pose: response.fields.pose,
      x: numberField(response.fields.x, "x"),
      y: numberField(response.fields.y, "y"),
      gazeX: numberField(response.fields.gazeX, "gazeX"),
      gazeY: numberField(response.fields.gazeY, "gazeY"),
      speaking: booleanField(response.fields.speaking, "speaking"),
      blinking: booleanField(response.fields.blinking, "blinking"),
    };
  }

  async setFace(expression: Expression): Promise<void> {
    const response = await this.send(`FACE:${expression}`);
    expectCommand(response.command, "FACE", response.raw);
  }

  async setLed(mood: Mood): Promise<void> {
    const response = await this.send(`LED:${mood}`);
    expectCommand(response.command, "LED", response.raw);
  }

  async setPose(pose: Pose): Promise<void> {
    const response = await this.send(`POSE:${pose}`);
    expectCommand(response.command, "POSE", response.raw);
  }

  async move(move: Pick<SafeMove, "x" | "y">): Promise<void> {
    const response = await this.send(`MOVE:${move.x}:${move.y}`);
    expectCommand(response.command, "MOVE", response.raw);
  }

  async reset(): Promise<void> {
    const response = await this.send("RESET");
    expectCommand(response.command, "RESET", response.raw);
  }

  async audioStatus(): Promise<AudioStatus> {
    const response = await this.send("AUDIO:STATUS");
    expectAudioResponse(response, "STATUS");
    return {
      state: response.fields.state ?? "Unknown",
      playing: booleanField(response.fields.playing, "playing") ?? false,
      volume: numberField(response.fields.volume, "volume") ?? 0,
      size: numberField(response.fields.size, "size") ?? 0,
      received: numberField(response.fields.received, "received") ?? 0,
    };
  }

  async setAudioVolume(volume: number): Promise<void> {
    const response = await this.send(`AUDIO:VOLUME:${volume}`);
    expectAudioResponse(response, "VOLUME");
  }

  async stopAudio(): Promise<void> {
    const response = await this.send("AUDIO:STOP");
    expectAudioResponse(response, "STOP");
  }

  async playWav(data: Buffer): Promise<void> {
    const { ready, final } = await this.client.sendCommandWithBinary(`AUDIO:WAV:${data.length}`, data);
    const readyResponse = parseSerialResponse(ready);
    if (readyResponse.kind !== "ready") {
      handleNonOkResponse(readyResponse);
      throw new BridgeError("AUDIO_TRANSFER_FAILED", `Expected READY response, got ${ready}`);
    }
    expectAudioResponse(readyResponse, "WAV");

    const finalResponse = parseSerialResponse(final);
    if (finalResponse.kind !== "ok") {
      handleNonOkResponse(finalResponse);
      throw new BridgeError("AUDIO_TRANSFER_FAILED", `Expected audio play response, got ${final}`);
    }
    expectAudioResponse(finalResponse, "PLAY");
  }

  private async send(command: string) {
    const raw = await this.client.sendCommand(command);
    const response = parseSerialResponse(raw);
    if (response.kind === "err") {
      handleNonOkResponse(response);
    }
    if (response.kind === "warn") {
      throw new BridgeError("STACKCHAN_ERROR", `StackChan returned warning ${response.code}`, response);
    }
    if (response.kind === "ready") {
      throw new BridgeError("PROTOCOL_PARSE_ERROR", `Unexpected READY response: ${response.raw}`, response);
    }
    return response;
  }
}

function expectCommand(actual: string, expected: string, raw: string): void {
  if (actual !== expected) {
    throw new BridgeError("PROTOCOL_PARSE_ERROR", `Expected ${expected} response, got ${actual}: ${raw}`);
  }
}

function expectAudioResponse(response: { command: string; args: string[]; raw: string }, expectedAction: string): void {
  expectCommand(response.command, "AUDIO", response.raw);
  if (response.args[0] !== expectedAction) {
    throw new BridgeError("PROTOCOL_PARSE_ERROR", `Expected AUDIO ${expectedAction} response: ${response.raw}`);
  }
}

function handleNonOkResponse(response: { kind: string; code?: string; raw: string; fields?: Record<string, string> }): never {
  const code = response.code ?? "AUDIO_TRANSFER_FAILED";
  if (
    code === "AUDIO_TOO_LARGE" ||
    code === "AUDIO_INVALID_FORMAT" ||
    code === "AUDIO_BUSY" ||
    code === "AUDIO_TRANSFER_FAILED" ||
    code === "AUDIO_RECEIVE_TIMEOUT"
  ) {
    throw new BridgeError(code as "AUDIO_TOO_LARGE" | "AUDIO_INVALID_FORMAT" | "AUDIO_BUSY" | "AUDIO_TRANSFER_FAILED" | "AUDIO_RECEIVE_TIMEOUT", `StackChan returned ${code}`, response);
  }
  throw new BridgeError("STACKCHAN_ERROR", `StackChan returned ${code}`, response);
}

function numberField(value: string | undefined, field: string): number | undefined {
  if (value === undefined) {
    return undefined;
  }
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) {
    throw new BridgeError("PROTOCOL_PARSE_ERROR", `${field} must be numeric: ${value}`);
  }
  return parsed;
}

function booleanField(value: string | undefined, field: string): boolean | undefined {
  if (value === undefined) {
    return undefined;
  }
  if (value === "true") {
    return true;
  }
  if (value === "false") {
    return false;
  }
  throw new BridgeError("PROTOCOL_PARSE_ERROR", `${field} must be boolean: ${value}`);
}
