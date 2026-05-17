import { stdin as input, stdout as output } from "node:process";
import { createInterface } from "node:readline/promises";
import { loadConfig } from "../config/loadConfig.js";
import { StackChanSerialClient } from "../serial/StackChanSerialClient.js";

type WifiSetup = {
  ssid: string;
  password: string;
  hostname: string;
  authToken: string;
};

const configPath = process.env.STACKCHAN_BRIDGE_CONFIG ?? process.argv[2] ?? "config.yaml";
const config = loadConfig(configPath);
const rl = createInterface({ input, output });

async function main(): Promise<void> {
  try {
    const setup = await askSetup();
    const json = JSON.stringify(setup);
    const payload = Buffer.from(json, "utf8");

    const client = new StackChanSerialClient(config.serial);
    console.log(`Opening serial port: ${config.serial.port}`);
    await client.connect();
    try {
      console.log(`Sending Wi-Fi setup to ${config.serial.port}`);
      console.log(`Payload size: ${payload.length} bytes`);
      console.log("Waiting for StackChan to save settings and connect Wi-Fi...");

      const { ready, final } = await client.sendCommandWithBinary(`WIFI:SET_JSON:${payload.length}`, payload);
      console.log(ready);
      console.log(final);

      const status = await client.sendCommand("WIFI:STATUS");
      console.log(status);
    } finally {
      await client.disconnect();
    }
  } finally {
    rl.close();
  }
}

async function askSetup(): Promise<WifiSetup> {
  const ssid = await askRequired("SSID: ");
  const password = await askHiddenRequired("Wi-Fi password: ");
  const hostname = (await rl.question("Hostname [stackchan-001]: ")).trim() || "stackchan-001";
  const authToken = await askHidden("Device auth token [empty = development mode]: ");

  return {
    ssid,
    password,
    hostname,
    authToken,
  };
}

async function askRequired(prompt: string): Promise<string> {
  const value = (await rl.question(prompt)).trim();
  if (value.length === 0) {
    throw new Error(`${prompt.trim()} is required`);
  }
  return value;
}

async function askHiddenRequired(prompt: string): Promise<string> {
  const value = await askHidden(prompt);
  if (value.length === 0) {
    throw new Error(`${prompt.trim()} is required`);
  }
  return value;
}

async function askHidden(prompt: string): Promise<string> {
  if (!input.isTTY) {
    return (await rl.question(prompt)).trim();
  }

  output.write(prompt);
  input.setRawMode(true);
  input.resume();

  return await new Promise<string>((resolve, reject) => {
    let value = "";

    const cleanup = () => {
      input.setRawMode(false);
      input.off("data", onData);
      output.write("\n");
    };

    const onData = (chunk: Buffer) => {
      const text = chunk.toString("utf8");
      for (const char of text) {
        if (char === "\u0003") {
          cleanup();
          reject(new Error("Cancelled"));
          return;
        }
        if (char === "\r" || char === "\n") {
          cleanup();
          resolve(value.trim());
          return;
        }
        if (char === "\u007f" || char === "\b") {
          value = value.slice(0, -1);
          continue;
        }
        value += char;
      }
    };

    input.on("data", onData);
  });
}

void main().catch((error: unknown) => {
  try {
    if (input.isTTY) {
      input.setRawMode(false);
    }
  } catch {
    // Ignore cleanup errors while reporting the original failure.
  }
  console.error(error instanceof Error ? error.message : String(error));
  process.exitCode = 1;
});
