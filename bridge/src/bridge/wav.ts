import { BridgeError } from "./BridgeError.js";

export const maxWavBytes = 1048576;

export function validateWav(buffer: Buffer): void {
  if (buffer.length === 0 || buffer.length > maxWavBytes) {
    throw new BridgeError("AUDIO_TOO_LARGE", "WAV file is too large", { max: maxWavBytes });
  }
  if (buffer.length < 44) {
    throw new BridgeError("AUDIO_INVALID_FORMAT", "WAV file is too small");
  }
  if (buffer.toString("ascii", 0, 4) !== "RIFF" || buffer.toString("ascii", 8, 12) !== "WAVE") {
    throw new BridgeError("AUDIO_INVALID_FORMAT", "WAV file must be RIFF/WAVE");
  }

  let offset = 12;
  let formatOk = false;
  let hasData = false;

  while (offset + 8 <= buffer.length) {
    const id = buffer.toString("ascii", offset, offset + 4);
    const size = buffer.readUInt32LE(offset + 4);
    const dataOffset = offset + 8;
    if (dataOffset + size > buffer.length) {
      throw new BridgeError("AUDIO_INVALID_FORMAT", "WAV chunk extends beyond file size");
    }

    if (id === "fmt ") {
      if (size < 16) {
        throw new BridgeError("AUDIO_INVALID_FORMAT", "WAV fmt chunk is too small");
      }
      const audioFormat = buffer.readUInt16LE(dataOffset);
      const channels = buffer.readUInt16LE(dataOffset + 2);
      const sampleRate = buffer.readUInt32LE(dataOffset + 4);
      const bitsPerSample = buffer.readUInt16LE(dataOffset + 14);
      formatOk =
        audioFormat === 1 &&
        channels === 1 &&
        (sampleRate === 16000 || sampleRate === 24000) &&
        bitsPerSample === 16;
    } else if (id === "data") {
      hasData = size > 0;
    }

    offset = dataOffset + size + (size % 2);
  }

  if (!formatOk || !hasData) {
    throw new BridgeError(
      "AUDIO_INVALID_FORMAT",
      "WAV must be PCM mono 16-bit at 16000Hz or 24000Hz with a data chunk",
    );
  }
}
