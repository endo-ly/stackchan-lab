import { StackChanBridge } from "./bridge/StackChanBridge.js";
import { loadConfig } from "./config/loadConfig.js";
import { createServer } from "./http/createServer.js";
import { MockStackChanClient } from "./mock/MockStackChanClient.js";
import { SerialProtocolClient } from "./serial/SerialProtocolClient.js";
import { StackChanSerialClient } from "./serial/StackChanSerialClient.js";
import type { StackChanClient } from "./serial/StackChanClient.js";

const configPath = process.env.STACKCHAN_BRIDGE_CONFIG ?? process.argv[2] ?? "config.yaml";
const config = loadConfig(configPath);
const deviceClient: StackChanClient =
  config.serial.mode === "mock" ? new MockStackChanClient() : new StackChanSerialClient(config.serial);
const protocol = new SerialProtocolClient(deviceClient);
const bridge = new StackChanBridge(config, protocol);
const server = await createServer(bridge);

await bridge.start();
await server.listen({
  host: config.server.host,
  port: config.server.port,
});

const shutdown = async () => {
  await server.close();
  await bridge.stop();
};

process.on("SIGINT", () => {
  void shutdown().then(() => process.exit(0));
});
process.on("SIGTERM", () => {
  void shutdown().then(() => process.exit(0));
});
