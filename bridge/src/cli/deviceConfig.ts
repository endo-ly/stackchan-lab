import { loadConfig } from "../config/loadConfig.js";
import { StackChanSerialClient } from "../serial/StackChanSerialClient.js";

type DeviceConfig = {
  speechServicesUrl: string;
  wakeAutoStart: boolean;
  wakeRecordDurationMs: number;
};

const configPath = process.env.STACKCHAN_BRIDGE_CONFIG ?? process.argv[2] ?? "config.yaml";
const config = loadConfig(configPath);

async function main(): Promise<void> {
  const deviceConfig: DeviceConfig = {
    speechServicesUrl: `${config.voiceGateway.baseUrl}/v1/transcribe`,
    wakeAutoStart: config.wake.autoStart,
    wakeRecordDurationMs: config.wake.recordDurationMs,
  };
  const json = JSON.stringify(deviceConfig);
  const payload = Buffer.from(json, "utf8");

  const client = new StackChanSerialClient(config.serial);
  console.log(`Opening serial port: ${config.serial.port}`);
  await client.connect();
  try {
    console.log("Sending device config");
    console.log(`Payload size: ${payload.length} bytes`);
    console.log(`speechServicesUrl: ${deviceConfig.speechServicesUrl}`);
    console.log(`wakeAutoStart: ${deviceConfig.wakeAutoStart}`);
    console.log(`wakeRecordDurationMs: ${deviceConfig.wakeRecordDurationMs}`);

    const { ready, final } = await client.sendCommandWithBinary(`DEVICE:CONFIG_JSON:${payload.length}`, payload);
    console.log(ready);
    console.log(final);
  } finally {
    await client.disconnect();
  }
}

void main().catch((error: unknown) => {
  console.error(error instanceof Error ? error.message : String(error));
  process.exitCode = 1;
});
