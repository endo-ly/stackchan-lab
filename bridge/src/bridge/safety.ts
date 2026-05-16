import { BridgeError } from "./BridgeError.js";
import type { BridgeConfig } from "../config/types.js";

export type MoveRequest = {
  x: number;
  y: number;
};

export type SafeMove = MoveRequest & {
  clamped: boolean;
  requested?: MoveRequest;
};

export function validateSafetyConfig(safety: BridgeConfig["safety"]): void {
  if (safety.minX > safety.maxX) {
    throw new BridgeError("CONFIG_ERROR", "safety.min_x must be less than or equal to safety.max_x");
  }
  if (safety.minY > safety.maxY) {
    throw new BridgeError("CONFIG_ERROR", "safety.min_y must be less than or equal to safety.max_y");
  }
}

export function clampMove(request: MoveRequest, safety: BridgeConfig["safety"]): SafeMove {
  const x = Math.min(Math.max(request.x, safety.minX), safety.maxX);
  const y = Math.min(Math.max(request.y, safety.minY), safety.maxY);
  const clamped = x !== request.x || y !== request.y;

  return clamped ? { x, y, clamped, requested: request } : { x, y, clamped };
}
