import { ReadlineParser, SerialPort } from "serialport";
import { BridgeError } from "../bridge/BridgeError.js";
import type { BridgeConfig } from "../config/types.js";
import type { StackChanClient } from "./StackChanClient.js";

export class StackChanSerialClient implements StackChanClient {
  private port?: SerialPort;
  private parser?: ReadlineParser;
  private commandQueue: Promise<unknown> = Promise.resolve();

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
    const queued = this.commandQueue.then(() => this.sendCommandNow(command));
    this.commandQueue = queued.catch(() => undefined);
    return queued;
  }

  async sendCommandLines(command: string, endPrefix: string): Promise<string[]> {
    const queued = this.commandQueue.then(() => this.sendCommandLinesNow(command, endPrefix));
    this.commandQueue = queued.catch(() => undefined);
    return queued;
  }

  async sendBinary(data: Buffer): Promise<string> {
    const queued = this.commandQueue.then(() => this.sendBinaryNow(data));
    this.commandQueue = queued.catch(() => undefined);
    return queued;
  }

  async sendCommandWithBinary(command: string, data: Buffer): Promise<{ ready: string; final: string }> {
    const queued = this.commandQueue.then(async () => {
      const ready = await this.sendCommandNow(command);
      const final = await this.sendBinaryNow(data);
      return { ready, final };
    });
    this.commandQueue = queued.catch(() => undefined);
    return queued;
  }

  private async sendCommandNow(command: string): Promise<string> {
    const { port, parser } = await this.requireConnection();

    return new Promise<string>((resolve, reject) => {
      const onData = (line: string) => {
        const trimmed = line.trim();
        if (!isCommandResponse(trimmed)) {
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
      this.flushThenWrite(port, command, cleanup, reject);
    });
  }

  private async sendCommandLinesNow(command: string, endPrefix: string): Promise<string[]> {
    const { port, parser } = await this.requireConnection();

    return new Promise<string[]>((resolve, reject) => {
      const lines: string[] = [];
      const onData = (line: string) => {
        const trimmed = line.trim();
        if (!isMultilineResponse(trimmed, endPrefix)) {
          return;
        }
        lines.push(trimmed);
        if (trimmed.startsWith("ERR ") || trimmed.startsWith(endPrefix)) {
          cleanup();
          resolve(lines);
          return;
        }
        if (lines.length === 1 && trimmed.startsWith("OK EVENTS count=0")) {
          cleanup();
          resolve(lines);
        }
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
        reject(new BridgeError("EVENTS_TIMEOUT", `Timed out waiting for multiline response to ${command}`));
      }, this.config.timeoutMs);

      parser.on("data", onData);
      port.on("error", onError);
      this.flushThenWrite(port, command, cleanup, reject);
    });
  }

  private async sendBinaryNow(data: Buffer): Promise<string> {
    if (!this.port || !this.parser || !this.isConnected()) {
      throw new BridgeError("STACKCHAN_NOT_CONNECTED", "StackChan serial port is not connected");
    }
    const port = this.port;
    const parser = this.parser;

    return new Promise<string>((resolve, reject) => {
      const onData = (line: string) => {
        const trimmed = line.trim();
        if (!isCommandResponse(trimmed)) {
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
        reject(new BridgeError("STACKCHAN_TIMEOUT", "Timed out waiting for audio transfer response"));
      }, Math.max(this.config.timeoutMs, 7000, Math.ceil(data.length / 10) + 5000));

      parser.on("data", onData);
      port.on("error", onError);
      void writeChunks(port, data).catch((error: unknown) => {
        cleanup();
        reject(new BridgeError("AUDIO_TRANSFER_FAILED", "Failed to write WAV data", error));
      });
    });
  }

  private async requireConnection(): Promise<{ port: SerialPort; parser: ReadlineParser }> {
    if (!this.isConnected()) {
      await this.tryReconnect();
    }
    if (!this.port || !this.parser || !this.isConnected()) {
      throw new BridgeError("STACKCHAN_NOT_CONNECTED", "StackChan serial port is not connected");
    }
    return { port: this.port, parser: this.parser };
  }

  private flushThenWrite(port: SerialPort, command: string, cleanup: () => void, reject: (reason?: unknown) => void): void {
    port.flush((flushError) => {
      if (flushError) {
        cleanup();
        reject(new BridgeError("STACKCHAN_NOT_CONNECTED", `Failed to flush serial port before command: ${command}`, flushError));
        return;
      }
      port.write(`${command}\n`, (writeError) => {
        if (writeError) {
          cleanup();
          reject(new BridgeError("STACKCHAN_NOT_CONNECTED", `Failed to write command: ${command}`, writeError));
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

async function writeChunks(port: SerialPort, data: Buffer): Promise<void> {
  const chunkSize = 128;
  for (let offset = 0; offset < data.length; offset += chunkSize) {
    const chunk = data.subarray(offset, Math.min(offset + chunkSize, data.length));
    await new Promise<void>((resolve, reject) => {
      port.write(chunk, (writeError) => {
        if (writeError) {
          reject(writeError);
          return;
        }
        port.drain((drainError) => {
          if (drainError) {
            reject(drainError);
            return;
          }
          resolve();
        });
      });
    });
    await new Promise((resolve) => setTimeout(resolve, 5));
  }
}

function isCommandResponse(line: string): boolean {
  return line.startsWith("OK ") || line.startsWith("READY ") || line.startsWith("ERR ") || line.startsWith("WARN ");
}

function isMultilineResponse(line: string, endPrefix: string): boolean {
  return isCommandResponse(line) || line.startsWith("EVENT ") || line.startsWith(endPrefix);
}
