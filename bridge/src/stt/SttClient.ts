import { BridgeError } from "../bridge/BridgeError.js";
import type { SttCapabilities, SttConfig, TranscriptionResult } from "./SttTypes.js";

type ApiResponse<T> =
  | { ok: true; data: T }
  | { ok: false; error: { code: string; message: string; details?: unknown } };

export class SttClient {
  constructor(private readonly config: SttConfig) {}

  async health() {
    if (!this.config.enabled) {
      return { enabled: false, reachable: false };
    }
    try {
      const data = await this.get<{ service: string; status: string; provider: string; modelLoaded: boolean }>("/health");
      return {
        enabled: true,
        reachable: true,
        service: {
          status: data.status,
          provider: data.provider,
          modelLoaded: data.modelLoaded,
        },
      };
    } catch (error) {
      return {
        enabled: true,
        reachable: false,
        service: error instanceof Error ? { status: error.message } : undefined,
      };
    }
  }

  async capabilities(): Promise<SttCapabilities> {
    this.assertEnabled();
    const data = await this.get<{ stt: SttCapabilities }>("/capabilities");
    return data.stt;
  }

  async transcribeFile(data: Buffer, filename: string): Promise<TranscriptionResult> {
    this.assertEnabled();
    const maxBytes = this.config.maxFileSizeMb * 1024 * 1024;
    if (data.length > maxBytes) {
      throw new BridgeError("AUDIO_TOO_LARGE", "Audio file exceeds STT size limit");
    }
    if (!filename.toLowerCase().endsWith(".wav")) {
      throw new BridgeError("AUDIO_INVALID_FORMAT", "Only WAV input is supported for STT");
    }

    const form = new FormData();
    form.set("file", new Blob([new Uint8Array(data)], { type: "audio/wav" }), filename);
    return await this.request<TranscriptionResult>("/transcribe", {
      method: "POST",
      body: form,
    });
  }

  private async get<T>(path: string): Promise<T> {
    return await this.request<T>(path, { method: "GET" });
  }

  private async request<T>(path: string, options: { method: "GET" | "POST"; body?: BodyInit }): Promise<T> {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), this.config.timeoutMs);

    try {
      const response = await fetch(`${this.config.baseUrl}${path}`, {
        method: options.method,
        body: options.body,
        signal: controller.signal,
      });
      const payload = (await response.json().catch(() => undefined)) as ApiResponse<T> | undefined;
      if (!response.ok) {
        throw new BridgeError("STT_ERROR", `speech-services returned HTTP ${response.status}`, payload);
      }
      if (!payload || payload.ok !== true) {
        const message = payload?.ok === false ? payload.error.message : "Invalid speech-services response";
        throw new BridgeError("STT_ERROR", message, payload);
      }
      return payload.data;
    } catch (error) {
      if (error instanceof BridgeError) {
        throw error;
      }
      if (error instanceof Error && error.name === "AbortError") {
        throw new BridgeError("STT_TIMEOUT", `Timed out calling speech-services: ${path}`, error);
      }
      throw new BridgeError("STT_SERVICE_UNREACHABLE", `Cannot reach speech-services: ${this.config.baseUrl}`, error);
    } finally {
      clearTimeout(timeout);
    }
  }

  private assertEnabled(): void {
    if (!this.config.enabled) {
      throw new BridgeError("STT_DISABLED", "STT is disabled");
    }
  }
}
