import { existsSync, readFileSync } from "node:fs";
import { resolve } from "node:path";
import YAML from "yaml";
import { BridgeError } from "../bridge/BridgeError.js";
import { validateSafetyConfig } from "../bridge/safety.js";
import type { BridgeConfig, DeviceTransportKind, SerialMode } from "./types.js";

type RawConfig = {
  server?: {
    host?: unknown;
    port?: unknown;
  };
  device?: {
    transport?: unknown;
  };
  serial?: {
    mode?: unknown;
    port?: unknown;
    baud_rate?: unknown;
    timeout_ms?: unknown;
    reconnect?: {
      enabled?: unknown;
      interval_ms?: unknown;
    };
  };
  wifi?: {
    base_url?: unknown;
    token?: unknown;
    timeout_ms?: unknown;
    health_check_interval_ms?: unknown;
  };
  safety?: {
    min_x?: unknown;
    max_x?: unknown;
    min_y?: unknown;
    max_y?: unknown;
  };
  logging?: {
    level?: unknown;
  };
};

export function loadConfig(configPath = "config.yaml"): BridgeConfig {
  const resolvedPath = resolve(configPath);
  const raw = existsSync(resolvedPath) ? parseConfigFile(resolvedPath) : {};

  const mode = stringValue(raw.serial?.mode, "real");
  if (mode !== "real" && mode !== "mock") {
    throw new BridgeError("CONFIG_ERROR", `serial.mode must be real or mock: ${mode}`);
  }
  const transport = stringValue(raw.device?.transport, "serial");
  if (transport !== "serial" && transport !== "wifi") {
    throw new BridgeError("CONFIG_ERROR", `device.transport must be serial or wifi: ${transport}`);
  }

  const config: BridgeConfig = {
    server: {
      host: stringValue(raw.server?.host, "127.0.0.1"),
      port: numberValue(raw.server?.port, 8787, "server.port"),
    },
    device: {
      transport: transport as DeviceTransportKind,
    },
    serial: {
      mode: mode as SerialMode,
      port: stringValue(raw.serial?.port, "/dev/stackchan"),
      baudRate: numberValue(raw.serial?.baud_rate, 115200, "serial.baud_rate"),
      timeoutMs: numberValue(raw.serial?.timeout_ms, 1000, "serial.timeout_ms"),
      reconnect: {
        enabled: booleanValue(raw.serial?.reconnect?.enabled, true, "serial.reconnect.enabled"),
        intervalMs: numberValue(raw.serial?.reconnect?.interval_ms, 2000, "serial.reconnect.interval_ms"),
      },
    },
    wifi: {
      baseUrl: normalizeBaseUrl(stringValue(raw.wifi?.base_url, "http://192.168.0.123")),
      token: optionalStringValue(raw.wifi?.token, "wifi.token"),
      timeoutMs: numberValue(raw.wifi?.timeout_ms, 5000, "wifi.timeout_ms"),
      healthCheckIntervalMs: numberValue(raw.wifi?.health_check_interval_ms, 5000, "wifi.health_check_interval_ms"),
    },
    safety: {
      minX: numberValue(raw.safety?.min_x, -300, "safety.min_x"),
      maxX: numberValue(raw.safety?.max_x, 300, "safety.max_x"),
      minY: numberValue(raw.safety?.min_y, 0, "safety.min_y"),
      maxY: numberValue(raw.safety?.max_y, 450, "safety.max_y"),
    },
    logging: {
      level: stringValue(raw.logging?.level, "info"),
    },
  };

  validateSafetyConfig(config.safety);
  return config;
}

function parseConfigFile(path: string): RawConfig {
  try {
    return YAML.parse(readFileSync(path, "utf8")) ?? {};
  } catch (error) {
    throw new BridgeError("CONFIG_ERROR", `Failed to read config: ${path}`, error);
  }
}

function stringValue(value: unknown, fallback: string): string {
  return typeof value === "string" ? value : fallback;
}

function optionalStringValue(value: unknown, field: string): string | undefined {
  if (value === undefined || value === null) {
    return undefined;
  }
  if (typeof value !== "string") {
    throw new BridgeError("CONFIG_ERROR", `${field} must be a string`);
  }
  return value.length > 0 ? value : undefined;
}

function numberValue(value: unknown, fallback: number, field: string): number {
  if (value === undefined) {
    return fallback;
  }
  if (typeof value !== "number" || !Number.isFinite(value)) {
    throw new BridgeError("CONFIG_ERROR", `${field} must be a finite number`);
  }
  return value;
}

function booleanValue(value: unknown, fallback: boolean, field: string): boolean {
  if (value === undefined) {
    return fallback;
  }
  if (typeof value !== "boolean") {
    throw new BridgeError("CONFIG_ERROR", `${field} must be a boolean`);
  }
  return value;
}

function normalizeBaseUrl(value: string): string {
  return value.endsWith("/") ? value.slice(0, -1) : value;
}
