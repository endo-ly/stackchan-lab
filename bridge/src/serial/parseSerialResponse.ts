import { BridgeError } from "../bridge/BridgeError.js";

export type SerialResponse =
  | {
      kind: "ok";
      command: string;
      args: string[];
      fields: Record<string, string>;
      raw: string;
    }
  | {
      kind: "err";
      code: string;
      fields: Record<string, string>;
      args: string[];
      raw: string;
    }
  | {
      kind: "warn";
      code: string;
      fields: Record<string, string>;
      args: string[];
      raw: string;
    }
  | {
      kind: "ready";
      command: string;
      args: string[];
      fields: Record<string, string>;
      raw: string;
    };

export function parseSerialResponse(line: string): SerialResponse {
  const trimmed = line.trim();
  const parts = trimmed.split(/\s+/).filter(Boolean);
  const prefix = parts[0];

  if (prefix === "OK") {
    const command = parts[1] ?? "";
    if (command.length === 0) {
      throw new BridgeError("PROTOCOL_PARSE_ERROR", `OK response is missing command: ${line}`);
    }
    const rest = parts.slice(2);
    return {
      kind: "ok",
      command,
      args: rest.filter((part) => !part.includes("=")),
      fields: parseFields(rest),
      raw: line,
    };
  }

  if (prefix === "READY") {
    const command = parts[1] ?? "";
    if (command.length === 0) {
      throw new BridgeError("PROTOCOL_PARSE_ERROR", `READY response is missing command: ${line}`);
    }
    const rest = parts.slice(2);
    return {
      kind: "ready",
      command,
      args: rest.filter((part) => !part.includes("=")),
      fields: parseFields(rest),
      raw: line,
    };
  }

  if (prefix === "ERR" || prefix === "WARN") {
    const code = parts[1] ?? "";
    if (code.length === 0) {
      throw new BridgeError("PROTOCOL_PARSE_ERROR", `${prefix} response is missing code: ${line}`);
    }
    const rest = parts.slice(2);
    return {
      kind: prefix === "ERR" ? "err" : "warn",
      code,
      fields: parseFields(rest),
      args: rest.filter((part) => !part.includes("=")),
      raw: line,
    };
  }

  throw new BridgeError("PROTOCOL_PARSE_ERROR", `Unknown response prefix: ${line}`);
}

function parseFields(parts: string[]): Record<string, string> {
  return Object.fromEntries(
    parts
      .map((part) => {
        const index = part.indexOf("=");
        return index > 0 ? [part.slice(0, index), part.slice(index + 1)] : undefined;
      })
      .filter((entry): entry is [string, string] => entry !== undefined),
  );
}
