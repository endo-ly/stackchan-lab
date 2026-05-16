import Fastify from "fastify";
import cors from "@fastify/cors";
import type { StackChanBridge } from "../bridge/StackChanBridge.js";
import { registerRoutes } from "./routes.js";

export async function createServer(bridge: StackChanBridge) {
  const server = Fastify({
    logger: true,
  });

  await server.register(cors, {
    origin: false,
  });
  await registerRoutes(server, bridge);

  return server;
}
