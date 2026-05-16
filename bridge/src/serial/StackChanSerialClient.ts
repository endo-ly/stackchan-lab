import { ReadlineParser, SerialPort } from "serialport";
import { BridgeError } from "../bridge/BridgeError.js";
import type { BridgeConfig } from "../config/types.js";
import type { StackChanClient } from "./StackChanClient.js";

export class StackChanSerialClient implements StackChanClient {
  private port?: SerialPort;
  private parser?: ReadlineParser;

  constructor(private readonly config: BridgeConfig["serial"]) {}

  async connect(): Promise<void> {
    if (this.isConnected()) {
      return;
    }

    this.port = new SerialPort({
      path: this.config.port,
      baudRate: this.config.baudRate,
      autoOpen: false,
    });
    this.parser = this.port.pipe(new ReadlineParser({ delimiter: "\n" }));

    await new Promise<void>((resolve, reject) => {
      this.port?.open((error) => {
        if (error) {
          reject(new BridgeError("STACKCHAN_NOT_CONNECTED", `Failed to open serial port ${this.config.port}`, error));
          return;
        }
        resolve();
      });
    });
  }

  async disconnect(): Promise<void> {
    if (!this.port || !this.port.isOpen) {
      return;
    }
    await new Promise<void>((resolve, reject) => {
      this.port?.close((error) => {
        if (error) {
          reject(error);
          return;
        }
        resolve();
      });
    });
  }

  isConnected(): boolean {
    return this.port?.isOpen === true;
  }

  async sendCommand(command: string): Promise<string> {
    if (!this.isConnected()) {
      await this.tryReconnect();
    }
    if (!this.port || !this.parser || !this.isConnected()) {
      throw new BridgeError("STACKCHAN_NOT_CONNECTED", "StackChan serial port is not connected");
    }
    const port = this.port;
    const parser = this.parser;

    return new Promise<string>((resolve, reject) => {
      const onData = (line: string) => {
        const trimmed = line.trim();
        if (trimmed.length === 0 || trimmed.startsWith("[")) {
          return;
        }
        cleanup();
        resolve(trimmed);
      };
      const onError = (error: Error) => {
        cleanup();
        reject(new BridgeError("STACKCHAN_NOT_CONNECTED", "Serial port error", error));
      };
      const cleanup = () => {
        clearTimeout(timer);
        parser.off("data", onData);
        port.off("error", onError);
      };
      const timer = setTimeout(() => {
        cleanup();
        reject(new BridgeError("STACKCHAN_TIMEOUT", `Timed out waiting for response to ${command}`));
      }, this.config.timeoutMs);

      parser.on("data", onData);
      port.on("error", onError);
      port.write(`${command}\n`, (error) => {
        if (error) {
          cleanup();
          reject(new BridgeError("STACKCHAN_NOT_CONNECTED", `Failed to write command: ${command}`, error));
        }
      });
    });
  }

  private async tryReconnect(): Promise<void> {
    if (!this.config.reconnect.enabled) {
      return;
    }
    await this.disconnect();
    await this.connect();
  }
}
