export type SerialMode = "real" | "mock";
export type DeviceTransportKind = "serial" | "wifi";

export type WifiConfig = {
  baseUrl: string;
  token?: string;
  timeoutMs: number;
  healthCheckIntervalMs: number;
};

export type BridgeConfig = {
  server: {
    host: string;
    port: number;
  };
  device: {
    transport: DeviceTransportKind;
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
  wifi: WifiConfig;
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
