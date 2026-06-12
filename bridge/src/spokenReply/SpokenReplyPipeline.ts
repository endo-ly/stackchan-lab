import type { BridgeConfig } from "../config/types.js";
import type { TranscriptionResult } from "../stt/SttTypes.js";
import type { DeviceTransport } from "../transport/DeviceTransport.js";
import type { VoiceGatewayClient } from "../voiceGateway/VoiceGatewayClient.js";

type AgentTurnClient = Pick<import("../agent/AgentClient.js").AgentClient, "createTurn">;
type SpeechSynthesizer = Pick<VoiceGatewayClient, "synthesizeSpeech">;

type SpokenReplyState = {
  enabled: boolean;
  busy: boolean;
  lastInput?: {
    source: string;
    text: string;
    timestamp: string;
  };
  lastReply?: {
    text: string;
    timestamp: string;
  };
  lastIgnored?: {
    source: string;
    reason: string;
    timestamp: string;
  };
  lastError?: {
    message: string;
    timestamp: string;
  };
  lastCompletedAt?: string;
  lastTiming?: {
    agentMs: number;
    ttsMs: number;
    playbackMs: number;
    totalMs: number;
  };
};

export class SpokenReplyPipeline {
  private enabled: boolean;
  private busy = false;
  private lastInput?: SpokenReplyState["lastInput"];
  private lastReply?: SpokenReplyState["lastReply"];
  private lastIgnored?: SpokenReplyState["lastIgnored"];
  private lastError?: SpokenReplyState["lastError"];
  private lastTiming?: SpokenReplyState["lastTiming"];
  private lastCompletedAtMs = 0;

  constructor(
    private readonly config: BridgeConfig,
    private readonly agentClient: AgentTurnClient,
    private readonly voiceGateway: SpeechSynthesizer,
    private readonly transport: DeviceTransport,
  ) {
    this.enabled = config.spokenReply.enabled;
  }

  start(): SpokenReplyState {
    this.enabled = true;
    return this.status();
  }

  stop(): SpokenReplyState {
    this.enabled = false;
    return this.status();
  }

  status(): SpokenReplyState {
    return {
      enabled: this.enabled,
      busy: this.busy,
      lastInput: this.lastInput,
      lastReply: this.lastReply,
      lastIgnored: this.lastIgnored,
      lastError: this.lastError,
      lastTiming: this.lastTiming,
      lastCompletedAt: this.lastCompletedAtMs > 0 ? new Date(this.lastCompletedAtMs).toISOString() : undefined,
    };
  }

  async handle(result: TranscriptionResult): Promise<void> {
    const now = Date.now();
    if (!this.enabled) {
      this.markIgnored(result.source, "disabled");
      return;
    }
    if (!this.config.spokenReply.listenSources.includes(result.source)) {
      this.markIgnored(result.source, "source_not_listened");
      return;
    }
    if (this.busy) {
      this.markIgnored(result.source, "busy");
      return;
    }
    if (this.lastCompletedAtMs > 0 && now - this.lastCompletedAtMs < this.config.spokenReply.cooldownMs) {
      this.markIgnored(result.source, "cooldown");
      return;
    }

    this.busy = true;
    this.lastError = undefined;
    this.lastReply = undefined;
    this.lastTiming = undefined;
    this.lastInput = {
      source: result.source,
      text: result.text,
      timestamp: new Date().toISOString(),
    };

    try {
      const totalStartedAt = performance.now();
      const agentStartedAt = performance.now();
      const replyText = await this.agentClient.createTurn(result.text, result.source);
      const agentMs = performance.now() - agentStartedAt;
      if (replyText.length === 0) {
        this.lastReply = {
          text: "",
          timestamp: new Date().toISOString(),
        };
        this.lastTiming = {
          agentMs: Math.round(agentMs),
          ttsMs: 0,
          playbackMs: 0,
          totalMs: Math.round(performance.now() - totalStartedAt),
        };
        this.lastCompletedAtMs = Date.now();
        return;
      }
      this.lastReply = {
        text: replyText,
        timestamp: new Date().toISOString(),
      };
      const ttsStartedAt = performance.now();
      const wav = await this.voiceGateway.synthesizeSpeech(replyText);
      const ttsMs = performance.now() - ttsStartedAt;
      const playbackStartedAt = performance.now();
      await this.transport.playWav(wav);
      const playbackMs = performance.now() - playbackStartedAt;
      this.lastTiming = {
        agentMs: Math.round(agentMs),
        ttsMs: Math.round(ttsMs),
        playbackMs: Math.round(playbackMs),
        totalMs: Math.round(performance.now() - totalStartedAt),
      };
      this.lastCompletedAtMs = Date.now();
    } catch (error) {
      this.lastError = {
        message: error instanceof Error ? error.message : String(error),
        timestamp: new Date().toISOString(),
      };
    } finally {
      this.busy = false;
    }
  }

  private markIgnored(source: string, reason: string): void {
    this.lastIgnored = {
      source,
      reason,
      timestamp: new Date().toISOString(),
    };
  }
}
