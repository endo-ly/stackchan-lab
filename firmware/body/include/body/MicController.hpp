#pragma once

#include <Arduino.h>

#include "body/MicState.hpp"

namespace stackchan {

constexpr uint32_t kMicSampleRate = 16000;
constexpr uint8_t kMicChannels = 1;
constexpr uint8_t kMicBitsPerSample = 16;
constexpr uint32_t kMaxMicRecordMs = 5000;

class MicController {
public:
    void begin();

    bool recordWav(uint32_t durationMs, String& error);
    const MicState& getState() const;
    const uint8_t* wavBuffer() const;
    size_t wavSize() const;

private:
    void releaseBuffer();
    bool allocateBuffer(size_t wavBytes, String& error);
    void writeWavHeader(size_t pcmBytes);

    MicState state_;
    uint8_t* wavBuffer_ = nullptr;
    size_t wavSize_ = 0;
};

}  // namespace stackchan
