export interface StackChanClient {
  connect(): Promise<void>;
  disconnect(): Promise<void>;
  isConnected(): boolean;
  sendCommand(command: string): Promise<string>;
  sendCommandLines(command: string, endPrefix: string): Promise<string[]>;
  sendBinary(data: Buffer): Promise<string>;
  sendCommandWithBinary(command: string, data: Buffer): Promise<{ ready: string; final: string }>;
}
