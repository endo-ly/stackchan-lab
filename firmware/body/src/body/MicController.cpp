#include "body/MicController.hpp"

#include <M5StackChan.h>

namespace stackchan {
namespace {

void writeLe16(uint8_t* data, uint16_t value)
{
    data[0] = value & 0xff;
    data[1] = (value >> 8) & 0xff;
}

void writeLe32(uint8_t* data, uint32_t value)
{
    data[0] = value & 0xff;
    data[1] = (value >> 8) & 0xff;
    data[2] = (value >> 16) & 0xff;
    data[3] = (value >> 24) & 0xff;
}

}  // namespace

void MicController::begin()
{
    M5.Mic.setSampleRate(kMicSampleRate);
    const bool ok = M5.Mic.begin();
    state_.setAvailable(ok && M5.Mic.isEnabled());
    state_.setFormat(kMicSampleRate, kMicChannels);
    if (!state_.available()) {
        state_.setError("MIC_NOT_AVAILABLE");
    }
}

void MicController::end()
{
    M5.Mic.end();
    state_.setAvailable(false);
    state_.setRecording(false);
}

bool MicController::recordWav(uint32_t durationMs, String& error)
{
    if (!state_.available()) {
        error = "MIC_NOT_AVAILABLE";
        state_.setError(error);
        return false;
    }
    if (durationMs == 0 || durationMs > kMaxMicRecordMs) {
        error = "MIC_INVALID_DURATION";
        state_.setError(error);
        return false;
    }

    const size_t samples = static_cast<size_t>(kMicSampleRate) * durationMs / 1000;
    const size_t pcmBytes = samples * sizeof(int16_t);
    const size_t wavBytes = 44 + pcmBytes;
    if (!allocateBuffer(wavBytes, error)) {
        state_.setError(error);
        return false;
    }

    state_.setRecording(true);
    state_.clearError();
    writeWavHeader(pcmBytes);
    const bool ok = M5.Mic.record(reinterpret_cast<int16_t*>(wavBuffer_ + 44), samples, kMicSampleRate, false);

    if (!ok) {
        state_.setRecording(false);
        error = "MIC_RECORD_FAILED";
        state_.setError(error);
        releaseBuffer();
        return false;
    }

    const uint32_t startedAt = millis();
    delay(durationMs + 50);
    while (M5.Mic.isRecording() && millis() - startedAt < durationMs + 1500) {
        delay(10);
    }
    state_.setRecording(false);

    if (M5.Mic.isRecording()) {
        error = "MIC_RECORD_TIMEOUT";
        state_.setError(error);
        releaseBuffer();
        return false;
    }

    wavSize_ = wavBytes;
    state_.setLastRecord(durationMs, pcmBytes, wavBytes);
    return true;
}

const MicState& MicController::getState() const
{
    return state_;
}

const uint8_t* MicController::wavBuffer() const
{
    return wavBuffer_;
}

size_t MicController::wavSize() const
{
    return wavSize_;
}

void MicController::releaseBuffer()
{
    if (wavBuffer_ != nullptr) {
        free(wavBuffer_);
        wavBuffer_ = nullptr;
    }
    wavSize_ = 0;
}

bool MicController::allocateBuffer(size_t wavBytes, String& error)
{
    releaseBuffer();
    wavBuffer_ = static_cast<uint8_t*>(ps_malloc(wavBytes));
    if (wavBuffer_ == nullptr) {
        wavBuffer_ = static_cast<uint8_t*>(malloc(wavBytes));
    }
    if (wavBuffer_ == nullptr) {
        error = "MIC_BUFFER_ALLOC_FAILED";
        return false;
    }
    wavSize_ = wavBytes;
    return true;
}

void MicController::writeWavHeader(size_t pcmBytes)
{
    memcpy(wavBuffer_, "RIFF", 4);
    writeLe32(wavBuffer_ + 4, static_cast<uint32_t>(36 + pcmBytes));
    memcpy(wavBuffer_ + 8, "WAVE", 4);
    memcpy(wavBuffer_ + 12, "fmt ", 4);
    writeLe32(wavBuffer_ + 16, 16);
    writeLe16(wavBuffer_ + 20, 1);
    writeLe16(wavBuffer_ + 22, kMicChannels);
    writeLe32(wavBuffer_ + 24, kMicSampleRate);
    writeLe32(wavBuffer_ + 28, kMicSampleRate * kMicChannels * kMicBitsPerSample / 8);
    writeLe16(wavBuffer_ + 32, kMicChannels * kMicBitsPerSample / 8);
    writeLe16(wavBuffer_ + 34, kMicBitsPerSample);
    memcpy(wavBuffer_ + 36, "data", 4);
    writeLe32(wavBuffer_ + 40, static_cast<uint32_t>(pcmBytes));
}

}  // namespace stackchan
