import Fastify from "fastify";
import cors from "@fastify/cors";
import multipart from "@fastify/multipart";
import type { StackChanBridge } from "../bridge/StackChanBridge.js";
import { registerRoutes } from "./routes.js";

export async function createServer(bridge: StackChanBridge) {
  const server = Fastify({
    logger: true,
  });

  await server.register(cors, {
    origin: false,
  });
  await server.register(multipart, {
    limits: {
      fileSize: 20 * 1024 * 1024,
      files: 1,
    },
  });
  await registerRoutes(server, bridge);

  return server;
}
