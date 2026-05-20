export type SttConfig = {
  enabled: boolean;
  baseUrl: string;
  timeoutMs: number;
  emitEvent: boolean;
  maxFileSizeMb: number;
};

export type SttHealth = {
  enabled: boolean;
  reachable: boolean;
  service?: {
    status?: string;
    provider?: string;
    modelLoaded?: boolean;
  };
};

export type SttCapabilities = {
  providers: string[];
  activeProvider: string;
  languages: string[];
  inputFormats: string[];
  maxAudioSeconds: number;
  streaming: boolean;
};

export type TranscriptionResult = {
  text: string;
  language: string;
  durationSec: number;
  processingMs: number;
  provider: string;
  model: string;
};

export type LatestTranscription = TranscriptionResult & {
  timestamp: string;
};

export type BridgeInputEvent = {
  id: number;
  source: "bridge";
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
  };
};
