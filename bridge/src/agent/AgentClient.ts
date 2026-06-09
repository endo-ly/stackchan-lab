import { BridgeError } from "../bridge/BridgeError.js";
import type { AgentRuntimeConfig } from "../config/types.js";

type AgentTurnResponse = {
  ok: boolean;
  response?: unknown;
};

export class AgentClient {
  constructor(private readonly config: AgentRuntimeConfig) {}

  async createTurn(text: string, source: string): Promise<string> {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), this.config.timeoutMs);

    try {
      const response = await fetch(this.config.endpoint, {
        method: "POST",
        headers: {
          Authorization: `Bearer ${this.config.authToken}`,
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          surface: this.config.surface,
          session_key: this.config.sessionKey,
          user_id: this.config.userId,
          text,
          source,
          agent_id: this.config.agentId,
        }),
        signal: controller.signal,
      });

      if (!response.ok) {
        throw new BridgeError(
          "AGENT_RUNTIME_ERROR",
          `agent runtime returned HTTP ${response.status}`,
          await response.text().catch(() => undefined),
        );
      }

      const payload = (await response.json().catch(() => undefined)) as AgentTurnResponse | undefined;
      if (payload?.ok !== true || typeof payload.response !== "string") {
        throw new BridgeError(
          "AGENT_RUNTIME_INVALID_RESPONSE",
          "agent runtime returned an invalid response",
          payload,
        );
      }
      return payload.response.trim();
    } catch (error) {
      if (error instanceof BridgeError) {
        throw error;
      }
      if (error instanceof Error && error.name === "AbortError") {
        throw new BridgeError("AGENT_RUNTIME_TIMEOUT", "Timed out calling agent runtime", error);
      }
      throw new BridgeError("AGENT_RUNTIME_UNREACHABLE", "Cannot reach agent runtime", error);
    } finally {
      clearTimeout(timeout);
    }
  }
}
