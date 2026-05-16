export type BridgeErrorCode =
  | "INVALID_REQUEST"
  | "INVALID_ARGUMENT"
  | "STACKCHAN_NOT_CONNECTED"
  | "STACKCHAN_TIMEOUT"
  | "STACKCHAN_ERROR"
  | "PROTOCOL_PARSE_ERROR"
  | "CONFIG_ERROR"
  | "INTERNAL_ERROR";

export class BridgeError extends Error {
  constructor(
    public readonly code: BridgeErrorCode,
    message: string,
    public readonly details?: unknown,
  ) {
    super(message);
  }
}
