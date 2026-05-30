import { BridgeError } from "../bridge/BridgeError.js";
import { validateWav } from "../bridge/wav.js";
import type { BridgeConfig } from "../config/types.js";

export class VoiceGatewayClient {
  constructor(private readonly config: BridgeConfig) {}

  async synthesizeSpeech(text: string): Promise<Buffer> {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), this.config.voiceGateway.timeoutMs);

    try {
      const response = await fetch(`${this.config.voiceGateway.baseUrl}/v1/audio/speech`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          model: this.config.tts.model,
          voice: this.config.tts.voice,
          input: text,
          response_format: this.config.tts.responseFormat,
        }),
        signal: controller.signal,
      });

      if (!response.ok) {
        throw new BridgeError(
          "VOICE_GATEWAY_ERROR",
          `voice-gateway returned HTTP ${response.status}`,
          await response.text().catch(() => undefined),
        );
      }

      const audio = Buffer.from(await response.arrayBuffer());
      validateWav(audio);
      return audio;
    } catch (error) {
      if (error instanceof BridgeError) {
        throw error;
      }
      if (error instanceof Error && error.name === "AbortError") {
        throw new BridgeError("VOICE_GATEWAY_TIMEOUT", "Timed out calling voice-gateway TTS", error);
      }
      throw new BridgeError("VOICE_GATEWAY_UNREACHABLE", "Cannot reach voice-gateway", error);
    } finally {
      clearTimeout(timeout);
    }
  }
}
