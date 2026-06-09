import assert from "node:assert/strict";
import { mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { loadConfig } from "./loadConfig.js";

test("loadConfig reads agent runtime settings", () => {
  const directory = mkdtempSync(join(tmpdir(), "stackchan-config-"));
  const path = join(directory, "config.yaml");
  writeFileSync(path, `
spoken_reply:
  enabled: true
agent_runtime:
  endpoint: http://127.0.0.1:10961/api/voice/turn
  auth_token: voice-secret
  agent_id: assistant
  surface: stackchan
  session_key: kitchen
  user_id: local-speaker
  timeout_ms: 30000
`);
  try {
    const config = loadConfig(path);
    assert.deepEqual(config.agentRuntime, {
      endpoint: "http://127.0.0.1:10961/api/voice/turn",
      authToken: "voice-secret",
      agentId: "assistant",
      surface: "stackchan",
      sessionKey: "kitchen",
      userId: "local-speaker",
      timeoutMs: 30000,
    });
  } finally {
    rmSync(directory, { recursive: true });
  }
});

test("loadConfig requires agent runtime when spoken reply is enabled", () => {
  const directory = mkdtempSync(join(tmpdir(), "stackchan-config-"));
  const path = join(directory, "config.yaml");
  writeFileSync(path, "spoken_reply:\n  enabled: true\n");
  try {
    assert.throws(() => loadConfig(path), /agent_runtime\.endpoint is required/);
  } finally {
    rmSync(directory, { recursive: true });
  }
});
