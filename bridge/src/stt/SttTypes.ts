export type SttConfig = {
  model: string;
};

export type TranscriptionResult = {
  source: string;
  text: string;
  language: string;
  durationSec: number;
  processingMs: number;
  provider: string;
  model: string;
  audio?: {
    sampleRate?: number;
    channels?: number;
    format?: string;
  };
};

export type LatestTranscription = TranscriptionResult & {
  timestamp: string;
};

export type BridgeInputEvent = {
  id: number;
  source: string;
  type: "stt";
  target: "audio";
  value: string;
  timestamp: string;
  metadata: {
    language: string;
    durationSec: number;
    processingMs: number;
    provider: string;
    model: string;
    audio?: {
      sampleRate?: number;
      channels?: number;
      format?: string;
    };
  };
};
