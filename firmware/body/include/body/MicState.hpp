#pragma once

#include <Arduino.h>

namespace stackchan {

class MicState {
public:
    bool available() const;
    bool recording() const;
    uint32_t sampleRate() const;
    uint8_t channels() const;
    uint32_t lastRecordMs() const;
    size_t lastPcmBytes() const;
    size_t lastWavBytes() const;
    const String& lastError() const;

    void setAvailable(bool available);
    void setRecording(bool recording);
    void setFormat(uint32_t sampleRate, uint8_t channels);
    void setLastRecord(uint32_t durationMs, size_t pcmBytes, size_t wavBytes);
    void setError(const String& error);
    void clearError();

private:
    bool available_ = false;
    bool recording_ = false;
    uint32_t sampleRate_ = 16000;
    uint8_t channels_ = 1;
    uint32_t lastRecordMs_ = 0;
    size_t lastPcmBytes_ = 0;
    size_t lastWavBytes_ = 0;
    String lastError_;
};

}  // namespace stackchan
