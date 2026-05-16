import type { Expression, Mood, Pose, PresetName } from "../types/body.js";

export type BodyPreset = {
  expression: Expression;
  mood: Mood;
  pose: Pose;
};

export const bodyPresets: Record<PresetName, BodyPreset> = {
  idle: { expression: "neutral", mood: "calm", pose: "neutral" },
  thinking: { expression: "doubt", mood: "calm", pose: "neutral" },
  alert: { expression: "neutral", mood: "warning", pose: "neutral" },
  listening: { expression: "neutral", mood: "active", pose: "neutral" },
  speaking: { expression: "neutral", mood: "speaking", pose: "neutral" },
  happy: { expression: "happy", mood: "active", pose: "neutral" },
  sleepy: { expression: "sleepy", mood: "calm", pose: "neutral" },
  error: { expression: "doubt", mood: "warning", pose: "neutral" },
};
