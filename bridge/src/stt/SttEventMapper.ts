import type { BridgeInputEvent, TranscriptionResult } from "./SttTypes.js";

export function transcriptionToEvent(id: number, result: TranscriptionResult, timestamp: string): BridgeInputEvent {
  return {
    id,
    source: "bridge",
    type: "stt",
    target: "audio",
    value: result.text,
    timestamp,
    metadata: {
      language: result.language,
      durationSec: result.durationSec,
      processingMs: result.processingMs,
      provider: result.provider,
      model: result.model,
    },
  };
}
