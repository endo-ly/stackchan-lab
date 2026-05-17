import { BridgeError } from "../bridge/BridgeError.js";
import type { WifiConfig } from "../config/types.js";
import type { SafeMove } from "../bridge/safety.js";
import type { AudioStatus, Expression, InputEvent, InputEvents, Mood, Pose } from "../types/body.js";
import type { ApiResponse } from "../types/api.js";
import type { DeviceStatus, DeviceTransport, DeviceVersionInfo } from "./DeviceTransport.js";

type JsonObject = Record<string, unknown>;

export class WifiTransport implements DeviceTransport {
  private connected = false;

  constructor(private readonly config: WifiConfig) {}

  async connect(): Promise<void> {
    await this.health();
    this.connected = true;
  }

  async disconnect(): Promise<void> {
    this.connected = false;
  }

  isConnected(): boolean {
    return this.connected;
  }

  async version(): Promise<DeviceVersionInfo> {
    return await this.get<DeviceVersionInfo>("/version");
  }

  async status(): Promise<DeviceStatus> {
    return await this.get<DeviceStatus>("/status");
  }

  async setFace(expression: Expression): Promise<void> {
    await this.post("/face", { expression });
  }

  async setLed(mood: Mood): Promise<void> {
    await this.post("/led", { mood });
  }

  async setPose(pose: Pose): Promise<void> {
    await this.post("/pose", { pose });
  }

  async move(move: Pick<SafeMove, "x" | "y">): Promise<void> {
    await this.post("/move", move);
  }

  async reset(): Promise<void> {
    await this.post("/reset", {});
  }

  async audioStatus(): Promise<AudioStatus> {
    return await this.get<AudioStatus>("/audio/status");
  }

  async setAudioVolume(volume: number): Promise<void> {
    await this.post("/audio/volume", { volume });
  }

  async stopAudio(): Promise<void> {
    await this.post("/audio/stop", {});
  }

  async playWav(data: Buffer): Promise<void> {
    await this.request<void>("/play-wav", {
      method: "POST",
      body: new Blob([new Uint8Array(data)], { type: "audio/wav" }),
      headers: {
        "Content-Type": "application/octet-stream",
      },
      timeoutMs: Math.max(this.config.timeoutMs, 15000),
    });
  }

  async events(): Promise<InputEvents> {
    return await this.get<InputEvents>("/events");
  }

  async latestEvent(): Promise<InputEvent | null> {
    const response = await this.get<{ event: InputEvent | null }>("/events/latest");
    return response.event;
  }

  async clearEvents(): Promise<void> {
    await this.post("/events/clear", {});
  }

  private async health(): Promise<void> {
    await this.get<JsonObject>("/health");
  }

  private async get<T>(path: string): Promise<T> {
    return await this.request<T>(path, { method: "GET" });
  }

  private async post<T>(path: string, body: JsonObject): Promise<T> {
    return await this.request<T>(path, {
      method: "POST",
      body: JSON.stringify(body),
      headers: {
        "Content-Type": "application/json",
      },
    });
  }

  private async request<T>(
    path: string,
    options: {
      method: "GET" | "POST";
      body?: BodyInit;
      headers?: Record<string, string>;
      timeoutMs?: number;
    },
  ): Promise<T> {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), options.timeoutMs ?? this.config.timeoutMs);

    try {
      const response = await fetch(`${this.config.baseUrl}${path}`, {
        method: options.method,
        body: options.body,
        headers: this.headers(options.headers),
        signal: controller.signal,
      });
      const payload = (await response.json().catch(() => undefined)) as ApiResponse<T> | undefined;
      if (response.status === 401) {
        throw new BridgeError("DEVICE_UNAUTHORIZED", "StackChan device rejected the Wi-Fi token", payload);
      }
      if (!response.ok) {
        throw new BridgeError("TRANSPORT_ERROR", `StackChan device returned HTTP ${response.status}`, payload);
      }
      if (!payload || payload.ok !== true) {
        const code = payload?.ok === false ? payload.error.code : "TRANSPORT_ERROR";
        throw new BridgeError(mapDeviceErrorCode(code), payload?.ok === false ? payload.error.message : "Invalid device response", payload);
      }
      this.connected = true;
      return payload.data;
    } catch (error) {
      this.connected = false;
      if (error instanceof BridgeError) {
        throw error;
      }
      if (error instanceof Error && error.name === "AbortError") {
        throw new BridgeError("DEVICE_TIMEOUT", `Timed out calling StackChan device: ${path}`, error);
      }
      throw new BridgeError("DEVICE_UNREACHABLE", `Cannot reach StackChan device: ${this.config.baseUrl}`, error);
    } finally {
      clearTimeout(timeout);
    }
  }

  private headers(extra?: Record<string, string>): Record<string, string> {
    return {
      ...(extra ?? {}),
      ...(this.config.token ? { "X-StackChan-Token": this.config.token } : {}),
    };
  }
}

function mapDeviceErrorCode(code: string): "DEVICE_UNAUTHORIZED" | "TRANSPORT_ERROR" {
  if (code === "UNAUTHORIZED") {
    return "DEVICE_UNAUTHORIZED";
  }
  return "TRANSPORT_ERROR";
}
