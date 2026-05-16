import type { FastifyInstance, FastifyReply } from "fastify";
import { BridgeError } from "../bridge/BridgeError.js";
import type { StackChanBridge } from "../bridge/StackChanBridge.js";

export async function registerRoutes(server: FastifyInstance, bridge: StackChanBridge): Promise<void> {
  server.get("/health", async () => success(bridge.getHealth()));
  server.get("/version", async () => success(await bridge.getVersion()));
  server.get("/status", async () => success(await bridge.getStatus()));
  server.get("/capabilities", async () => success(bridge.getCapabilities()));
  server.get("/audio/status", async () => success(await bridge.getAudioStatus()));
  server.get("/presets", async () => success(bridge.getPresets()));

  server.post("/face", async (request) => {
    const body = requireObject(request.body);
    return success(await bridge.setFace(requireString(body.expression, "expression")));
  });

  server.post("/led", async (request) => {
    const body = requireObject(request.body);
    return success(await bridge.setLed(requireString(body.mood, "mood")));
  });

  server.post("/pose", async (request) => {
    const body = requireObject(request.body);
    return success(await bridge.setPose(requireString(body.pose, "pose")));
  });

  server.post("/move", async (request) => {
    const body = requireObject(request.body);
    return success(await bridge.move(requireNumber(body.x, "x"), requireNumber(body.y, "y")));
  });

  server.post("/reset", async () => success(await bridge.reset()));

  server.post("/audio/volume", async (request) => {
    const body = requireObject(request.body);
    return success(await bridge.setAudioVolume(requireNumber(body.volume, "volume")));
  });

  server.post("/audio/stop", async () => success(await bridge.stopAudio()));

  server.post("/preset", async (request) => {
    const body = requireObject(request.body);
    return success(await bridge.applyPreset(requireString(body.name, "name")));
  });

  server.post("/play-wav", async (request) => {
    const file = await request.file();
    if (!file) {
      throw new BridgeError("INVALID_REQUEST", "multipart file field is required");
    }
    return success(await bridge.playWav(await file.toBuffer()));
  });

  server.setErrorHandler((error: Error, _request, reply) => {
    sendError(reply, error);
  });
}

function success<T>(data: T) {
  return { ok: true, data };
}

function requireObject(value: unknown): Record<string, unknown> {
  if (value === null || typeof value !== "object" || Array.isArray(value)) {
    throw new BridgeError("INVALID_REQUEST", "Request body must be a JSON object");
  }
  return value as Record<string, unknown>;
}

function requireString(value: unknown, field: string): string {
  if (typeof value !== "string" || value.length === 0) {
    throw new BridgeError("INVALID_ARGUMENT", `${field} must be a non-empty string`);
  }
  return value;
}

function requireNumber(value: unknown, field: string): number {
  if (typeof value !== "number" || !Number.isFinite(value)) {
    throw new BridgeError("INVALID_ARGUMENT", `${field} must be a finite number`);
  }
  return value;
}

function sendError(reply: FastifyReply, error: Error): void {
  if (error instanceof BridgeError) {
    reply.status(statusFor(error.code)).send({
      ok: false,
      error: {
        code: error.code,
        message: error.message,
        details: error.details,
      },
    });
    return;
  }

  reply.status(500).send({
    ok: false,
    error: {
      code: "INTERNAL_ERROR",
      message: error.message,
    },
  });
}

function statusFor(code: string): number {
  switch (code) {
    case "INVALID_REQUEST":
    case "INVALID_ARGUMENT":
    case "CONFIG_ERROR":
    case "PROTOCOL_PARSE_ERROR":
    case "AUDIO_TOO_LARGE":
    case "AUDIO_INVALID_FORMAT":
    case "UNSUPPORTED_PRESET":
      return 400;
    case "STACKCHAN_NOT_CONNECTED":
      return 503;
    case "STACKCHAN_TIMEOUT":
    case "AUDIO_RECEIVE_TIMEOUT":
      return 504;
    case "AUDIO_BUSY":
      return 503;
    case "STACKCHAN_ERROR":
    case "AUDIO_TRANSFER_FAILED":
    case "PRESET_APPLY_FAILED":
      return 500;
    default:
      return 500;
  }
}
