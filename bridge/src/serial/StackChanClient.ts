export interface StackChanClient {
  connect(): Promise<void>;
  disconnect(): Promise<void>;
  isConnected(): boolean;
  sendCommand(command: string): Promise<string>;
}
