export type BridgeErrorCode =
  | "INVALID_REQUEST"
  | "INVALID_ARGUMENT"
  | "STACKCHAN_NOT_CONNECTED"
  | "STACKCHAN_TIMEOUT"
  | "STACKCHAN_ERROR"
  | "DEVICE_UNREACHABLE"
  | "DEVICE_TIMEOUT"
  | "DEVICE_UNAUTHORIZED"
  | "TRANSPORT_ERROR"
  | "TRANSPORT_NOT_CONFIGURED"
  | "PROTOCOL_PARSE_ERROR"
  | "EVENT_PARSE_ERROR"
  | "EVENTS_TIMEOUT"
  | "EVENT_QUEUE_ERROR"
  | "UNSUPPORTED_INPUT"
  | "CONFIG_ERROR"
  | "AUDIO_TOO_LARGE"
  | "AUDIO_INVALID_FORMAT"
  | "AUDIO_BUSY"
  | "AUDIO_TRANSFER_FAILED"
  | "AUDIO_RECEIVE_TIMEOUT"
  | "UNSUPPORTED_PRESET"
  | "PRESET_APPLY_FAILED"
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
