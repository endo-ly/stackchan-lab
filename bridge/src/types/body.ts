export const expressions = ["neutral", "happy", "sad", "angry", "sleepy", "doubt"] as const;
export const moods = ["calm", "active", "speaking", "warning", "off"] as const;
export const poses = ["neutral", "look_left", "look_right", "look_up", "look_down"] as const;
export const commands = ["PING", "VERSION", "HELP", "STATUS", "FACE", "LED", "POSE", "MOVE", "RESET", "AUDIO", "EVENTS"] as const;
export const presets = ["idle", "thinking", "alert", "listening", "speaking", "happy", "sleepy", "error"] as const;

export type Expression = (typeof expressions)[number];
export type Mood = (typeof moods)[number];
export type Pose = (typeof poses)[number];
export type PresetName = (typeof presets)[number];

export type AudioStatus = {
  state: string;
  playing: boolean;
  volume: number;
  size: number;
  received: number;
};

export type InputEvent = {
  id: number;
  type: string;
  target: string;
  value: string;
  timestamp: number;
  x?: number;
  y?: number;
};

export type ReceivedInputEvent = InputEvent & {
  receivedAt: string;
};

export type InputEvents = {
  count: number;
  events: InputEvent[];
};

export type BodyStatus = {
  connected: boolean;
  mode?: string;
  expression?: string;
  mood?: string;
  pose?: string;
  x?: number;
  y?: number;
  gazeX?: number;
  gazeY?: number;
  speaking?: boolean;
  blinking?: boolean;
  eventCount?: number;
  latestEvent?: string;
  latestTarget?: string;
  latestValue?: string;
  touchActive?: boolean;
  buttonActive?: boolean;
  imuMoving?: boolean;
  lastCommandAt?: string;
  lastResponseAt?: string;
};

export type DeviceVersion = {
  connected: boolean;
  firmware?: string;
  protocol?: string;
  board?: string;
};
