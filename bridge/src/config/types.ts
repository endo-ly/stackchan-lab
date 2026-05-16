export type SerialMode = "real" | "mock";

export type BridgeConfig = {
  server: {
    host: string;
    port: number;
  };
  serial: {
    mode: SerialMode;
    port: string;
    baudRate: number;
    timeoutMs: number;
    reconnect: {
      enabled: boolean;
      intervalMs: number;
    };
  };
  safety: {
    minX: number;
    maxX: number;
    minY: number;
    maxY: number;
  };
  logging: {
    level: string;
  };
};
