import assert from "node:assert/strict";
import { createServer } from "node:http";
import test from "node:test";
import { AgentClient } from "./AgentClient.js";

test("AgentClient sends the configured voice turn contract", async () => {
  let receivedBody: unknown;
  let receivedAuth: string | undefined;
  const server = createServer((request, response) => {
    receivedAuth = request.headers.authorization;
    const chunks: Buffer[] = [];
    request.on("data", (chunk) => chunks.push(chunk));
    request.on("end", () => {
      receivedBody = JSON.parse(Buffer.concat(chunks).toString("utf8"));
      response.writeHead(200, { "Content-Type": "application/json" });
      response.end(JSON.stringify({ ok: true, response: "聞こえています" }));
    });
  });
  await new Promise<void>((resolve) => server.listen(0, "127.0.0.1", resolve));
  const address = server.address();
  assert(address && typeof address !== "string");

  try {
    const client = new AgentClient({
      endpoint: `http://127.0.0.1:${address.port}/api/voice/turn`,
      authToken: "voice-secret",
      agentId: "default",
      surface: "stackchan",
      sessionKey: "main",
      userId: "local-speaker",
      timeoutMs: 1000,
    });

    assert.equal(await client.createTurn("こんにちは", "stackchan-wake"), "聞こえています");
    assert.equal(receivedAuth, "Bearer voice-secret");
    assert.deepEqual(receivedBody, {
      surface: "stackchan",
      session_key: "main",
      user_id: "local-speaker",
      text: "こんにちは",
      source: "stackchan-wake",
      agent_id: "default",
    });
  } finally {
    server.close();
  }
});

test("AgentClient accepts an empty successful response", async () => {
  const server = createServer((_request, response) => {
    response.writeHead(200, { "Content-Type": "application/json" });
    response.end(JSON.stringify({ ok: true, response: "  " }));
  });
  await new Promise<void>((resolve) => server.listen(0, "127.0.0.1", resolve));
  const address = server.address();
  assert(address && typeof address !== "string");

  try {
    const client = new AgentClient({
      endpoint: `http://127.0.0.1:${address.port}/api/voice/turn`,
      authToken: "voice-secret",
      agentId: "default",
      surface: "stackchan",
      sessionKey: "main",
      userId: "local-speaker",
      timeoutMs: 1000,
    });
    assert.equal(await client.createTurn("silence", "stackchan-wake"), "");
  } finally {
    server.close();
  }
});

test("AgentClient maps non-success responses to an agent runtime error", async () => {
  const server = createServer((_request, response) => {
    response.writeHead(503, { "Content-Type": "application/json" });
    response.end(JSON.stringify({ ok: false, error: "turn_failed" }));
  });
  await new Promise<void>((resolve) => server.listen(0, "127.0.0.1", resolve));
  const address = server.address();
  assert(address && typeof address !== "string");

  try {
    const client = new AgentClient({
      endpoint: `http://127.0.0.1:${address.port}/api/voice/turn`,
      authToken: "voice-secret",
      agentId: "default",
      surface: "stackchan",
      sessionKey: "main",
      userId: "local-speaker",
      timeoutMs: 1000,
    });
    await assert.rejects(
      client.createTurn("failure", "stackchan-wake"),
      (error: unknown) =>
        error instanceof Error
        && "code" in error
        && error.code === "AGENT_RUNTIME_ERROR",
    );
  } finally {
    server.close();
  }
});

test("AgentClient aborts after the configured timeout", async () => {
  const server = createServer(() => {});
  await new Promise<void>((resolve) => server.listen(0, "127.0.0.1", resolve));
  const address = server.address();
  assert(address && typeof address !== "string");

  try {
    const client = new AgentClient({
      endpoint: `http://127.0.0.1:${address.port}/api/voice/turn`,
      authToken: "voice-secret",
      agentId: "default",
      surface: "stackchan",
      sessionKey: "main",
      userId: "local-speaker",
      timeoutMs: 10,
    });
    await assert.rejects(
      client.createTurn("timeout", "stackchan-wake"),
      (error: unknown) =>
        error instanceof Error
        && "code" in error
        && error.code === "AGENT_RUNTIME_TIMEOUT",
    );
  } finally {
    server.closeAllConnections();
    server.close();
  }
});
