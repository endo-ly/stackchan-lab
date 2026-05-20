#include "body/MicState.hpp"

namespace stackchan {

bool MicState::available() const { return available_; }
bool MicState::recording() const { return recording_; }
uint32_t MicState::sampleRate() const { return sampleRate_; }
uint8_t MicState::channels() const { return channels_; }
uint32_t MicState::lastRecordMs() const { return lastRecordMs_; }
size_t MicState::lastPcmBytes() const { return lastPcmBytes_; }
size_t MicState::lastWavBytes() const { return lastWavBytes_; }
const String& MicState::lastError() const { return lastError_; }

void MicState::setAvailable(bool available) { available_ = available; }
void MicState::setRecording(bool recording) { recording_ = recording; }
void MicState::setFormat(uint32_t sampleRate, uint8_t channels)
{
    sampleRate_ = sampleRate;
    channels_ = channels;
}
void MicState::setLastRecord(uint32_t durationMs, size_t pcmBytes, size_t wavBytes)
{
    lastRecordMs_ = durationMs;
    lastPcmBytes_ = pcmBytes;
    lastWavBytes_ = wavBytes;
}
void MicState::setError(const String& error) { lastError_ = error; }
void MicState::clearError() { lastError_ = ""; }

}  // namespace stackchan
