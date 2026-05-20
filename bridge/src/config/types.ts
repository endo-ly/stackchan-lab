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
  stt: {
    enabled: boolean;
    baseUrl: string;
    timeoutMs: number;
    emitEvent: boolean;
    maxFileSizeMb: number;
  };
  events: {
    includeBridgeEvents: boolean;
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
