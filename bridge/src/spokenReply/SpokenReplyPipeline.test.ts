import assert from "node:assert/strict";
import test from "node:test";
import type { BridgeConfig } from "../config/types.js";
import type { DeviceTransport } from "../transport/DeviceTransport.js";
import { SpokenReplyPipeline } from "./SpokenReplyPipeline.js";

const config = {
  spokenReply: {
    enabled: true,
    listenSources: ["stackchan-wake"],
    cooldownMs: 0,
    busyPolicy: "ignore",
  },
} as BridgeConfig;

const transcription = {
  source: "stackchan-wake",
  text: "こんにちは",
  language: "ja",
  durationSec: 1,
  processingMs: 100,
  provider: "reazon",
  model: "stt-default",
};

test("SpokenReplyPipeline speaks the agent response", async () => {
  const calls: string[] = [];
  const pipeline = new SpokenReplyPipeline(
    config,
    {
      createTurn: async (text, source) => {
        calls.push(`agent:${source}:${text}`);
        return "agent response";
      },
    },
    {
      synthesizeSpeech: async (text) => {
        calls.push(`tts:${text}`);
        return Buffer.from("wav");
      },
    },
    {
      playWav: async () => {
        calls.push("playback");
      },
    } as unknown as DeviceTransport,
  );

  await pipeline.handle(transcription);

  assert.deepEqual(calls, [
    "agent:stackchan-wake:こんにちは",
    "tts:agent response",
    "playback",
  ]);
  assert.equal(pipeline.status().lastReply?.text, "agent response");
  assert(pipeline.status().lastTiming);
});

test("SpokenReplyPipeline skips TTS and playback for an empty response", async () => {
  let downstreamCalls = 0;
  const pipeline = new SpokenReplyPipeline(
    config,
    { createTurn: async () => "" },
    {
      synthesizeSpeech: async () => {
        downstreamCalls += 1;
        return Buffer.from("wav");
      },
    },
    {
      playWav: async () => {
        downstreamCalls += 1;
      },
    } as unknown as DeviceTransport,
  );

  await pipeline.handle(transcription);

  assert.equal(downstreamCalls, 0);
  assert.equal(pipeline.status().lastError, undefined);
  assert.equal(pipeline.status().lastTiming?.ttsMs, 0);
});

test("SpokenReplyPipeline records an agent error without speaking", async () => {
  let downstreamCalls = 0;
  const pipeline = new SpokenReplyPipeline(
    config,
    { createTurn: async () => { throw new Error("agent unavailable"); } },
    {
      synthesizeSpeech: async () => {
        downstreamCalls += 1;
        return Buffer.from("wav");
      },
    },
    {
      playWav: async () => {
        downstreamCalls += 1;
      },
    } as unknown as DeviceTransport,
  );

  await pipeline.handle(transcription);

  assert.equal(downstreamCalls, 0);
  assert.equal(pipeline.status().busy, false);
  assert.equal(pipeline.status().lastError?.message, "agent unavailable");
  assert.equal(pipeline.status().lastTiming, undefined);
});

test("SpokenReplyPipeline does not call the agent for an ignored source", async () => {
  let agentCalls = 0;
  const pipeline = new SpokenReplyPipeline(
    config,
    {
      createTurn: async () => {
        agentCalls += 1;
        return "unused";
      },
    },
    { synthesizeSpeech: async () => Buffer.from("wav") },
    { playWav: async () => {} } as unknown as DeviceTransport,
  );

  await pipeline.handle({ ...transcription, source: "manual-record" });

  assert.equal(agentCalls, 0);
  assert.equal(pipeline.status().lastIgnored?.reason, "source_not_listened");
});
