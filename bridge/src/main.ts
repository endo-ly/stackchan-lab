import { StackChanBridge } from "./bridge/StackChanBridge.js";
import { loadConfig } from "./config/loadConfig.js";
import { createServer } from "./http/createServer.js";
import { MockStackChanClient } from "./mock/MockStackChanClient.js";
import { SerialProtocolClient } from "./serial/SerialProtocolClient.js";
import { StackChanSerialClient } from "./serial/StackChanSerialClient.js";
import type { StackChanClient } from "./serial/StackChanClient.js";
import { SpokenReplyPipeline } from "./spokenReply/SpokenReplyPipeline.js";
import type { DeviceTransport } from "./transport/DeviceTransport.js";
import { SerialTransport } from "./transport/SerialTransport.js";
import { WifiTransport } from "./transport/WifiTransport.js";
import { VoiceGatewayClient } from "./voiceGateway/VoiceGatewayClient.js";

const configPath = process.env.STACKCHAN_BRIDGE_CONFIG ?? process.argv[2] ?? "config.yaml";
const config = loadConfig(configPath);
const transport = createTransport();
const voiceGateway = new VoiceGatewayClient(config);
const spokenReply = new SpokenReplyPipeline(config, voiceGateway, transport);
const bridge = new StackChanBridge(config, transport, spokenReply);
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

function createTransport(): DeviceTransport {
  if (config.serial.mode === "mock") {
    return new SerialTransport(new SerialProtocolClient(new MockStackChanClient()));
  }
  if (config.device.transport === "wifi") {
    return new WifiTransport(config.wifi);
  }
  const deviceClient: StackChanClient = new StackChanSerialClient(config.serial);
  return new SerialTransport(new SerialProtocolClient(deviceClient));
}
