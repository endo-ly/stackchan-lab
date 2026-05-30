#include "body/AudioController.hpp"

#include <M5StackChan.h>

namespace stackchan {
namespace {

uint16_t readLe16(const uint8_t* data)
{
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t readLe32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0])
        | (static_cast<uint32_t>(data[1]) << 8)
        | (static_cast<uint32_t>(data[2]) << 16)
        | (static_cast<uint32_t>(data[3]) << 24);
}

}  // namespace

void AudioController::begin()
{
    mode_ = AudioMode::Idle;
    auto config = M5.Speaker.config();
    config.dma_buf_len = 1024;
    config.dma_buf_count = 8;
    M5.Speaker.config(config);
    // Speaker is NOT begin()-ed here; it will be lazily initialized
    // in rebeginSpeakerForWav() right before playback.
    M5.Speaker.setVolume(state_.volume());
    M5.Speaker.setAllChannelVolume(255);
    state_.setState(AudioPlaybackState::Idle);
    state_.setPlaying(false);
    state_.setQueued(false);
    expectedDurationMs_ = 0;
    wavSampleRate_ = 0;
    Serial.println("[AUDIO] begin done (lazy init)");
}

void AudioController::update()
{
    if (mode_ == AudioMode::Playback && state_.isQueued()) {
        return;
    }

    if (state_.isPlaying()) {
        const bool speakerStillPlaying = M5.Speaker.isPlaying();
        const uint32_t elapsed = millis() - state_.startedAt();
        const bool timeExpired = (expectedDurationMs_ > 0) && (elapsed > expectedDurationMs_ + 500);

        if (!speakerStillPlaying || timeExpired) {
            if (timeExpired) {
                Serial.println("[AUDIO] failsafe: forcing stop after timeout");
                M5.Speaker.stop();
            }
            state_.setPlaying(false);
            state_.setState(AudioPlaybackState::Finished);
            state_.markFinished(millis());
            releaseBuffer();
        }
    }
}

AudioMode AudioController::mode() const
{
    return mode_;
}

void AudioController::setMode(AudioMode mode)
{
    mode_ = mode;
}

bool AudioController::prepareWav(size_t size, String& error)
{
    if (state_.isPlaying()) {
        error = "AUDIO_BUSY";
        return false;
    }
    if (size == 0 || size > kMaxWavBytes) {
        error = "AUDIO_TOO_LARGE";
        return false;
    }

    releaseBuffer();
    wavBuffer_ = static_cast<uint8_t*>(ps_malloc(size));
    if (wavBuffer_ == nullptr) {
        wavBuffer_ = static_cast<uint8_t*>(malloc(size));
    }
    if (wavBuffer_ == nullptr) {
        error = "INTERNAL_ERROR";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        return false;
    }

    state_.setState(AudioPlaybackState::Receiving);
    state_.setCurrentSize(size);
    state_.setReceivedSize(0);
    state_.clearError();
    return true;
}

uint8_t* AudioController::preparedBuffer()
{
    return wavBuffer_;
}

bool AudioController::queuePlayWav(uint8_t* buffer, size_t size, String& error)
{
    if (buffer == nullptr || buffer != wavBuffer_ || size != state_.currentSize()) {
        error = "AUDIO_TRANSFER_FAILED";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        return false;
    }
    state_.setReceivedSize(size);

    if (!validateWav(buffer, size)) {
        error = "AUDIO_INVALID_FORMAT";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        releaseBuffer();
        return false;
    }

    wavSampleRate_ = extractSampleRate(buffer, size);
    expectedDurationMs_ = estimateWavDurationMs(buffer, size);

    state_.setState(AudioPlaybackState::Queued);
    state_.setQueued(true);
    return true;
}

bool AudioController::startQueuedPlay(String& error)
{
    if (!state_.isQueued()) {
        error = "AUDIO_NOT_QUEUED";
        return false;
    }

    if (wavBuffer_ == nullptr) {
        error = "AUDIO_BUFFER_LOST";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        state_.setQueued(false);
        return false;
    }

    const size_t size = state_.receivedSize();

    logHardwareState("before playWav");

    if (!M5.Speaker.playWav(wavBuffer_, size, 1, -1, true)) {
        error = "AUDIO_INVALID_FORMAT";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        state_.setQueued(false);
        releaseBuffer();
        return false;
    }

    state_.setState(AudioPlaybackState::Playing);
    state_.setPlaying(true);
    state_.setQueued(false);
    state_.markStarted(millis());
    state_.clearError();
    setMode(AudioMode::Playback);
    logHardwareState("after playWav");
    return true;
}

bool AudioController::rebeginSpeakerForWav(uint32_t sampleRate)
{
    Serial.println("[AUDIO] rebeginSpeakerForWav: stop + config + begin");

    if (M5.Speaker.isPlaying()) {
        Serial.println("[AUDIO] speaker still playing, stopping first");
        M5.Speaker.stop();
        delay(30);
    }

    auto config = M5.Speaker.config();
    config.sample_rate = sampleRate > 0 ? sampleRate : 24000;
    config.dma_buf_len = 1024;
    config.dma_buf_count = 8;
    M5.Speaker.config(config);

    const bool ok = M5.Speaker.begin();

    M5.Speaker.setVolume(state_.volume());
    M5.Speaker.setAllChannelVolume(255);

    logHardwareState("after rebegin (config + begin)");
    return ok && M5.Speaker.isEnabled() && M5.Speaker.isRunning();
}

void AudioController::logHardwareState(const char* label) const
{
    Serial.printf(
        "[AUDIO] %s: spk enabled=%d running=%d playing=%d | mic enabled=%d running=%d recording=%d\n",
        label,
        M5.Speaker.isEnabled(),
        M5.Speaker.isRunning(),
        M5.Speaker.isPlaying(),
        M5.Mic.isEnabled(),
        M5.Mic.isRunning(),
        M5.Mic.isRecording()
    );
}

uint32_t AudioController::preparedSampleRate() const
{
    return wavSampleRate_;
}

void AudioController::stop()
{
    M5.Speaker.stop();
    state_.setPlaying(false);
    state_.setQueued(false);
    state_.setState(AudioPlaybackState::Idle);
    state_.setCurrentSize(0);
    state_.setReceivedSize(0);
    expectedDurationMs_ = 0;
    wavSampleRate_ = 0;
    state_.markFinished(millis());
    releaseBuffer();
    setMode(AudioMode::Idle);
}

bool AudioController::setVolume(int volume)
{
    if (volume < 0 || volume > 255) {
        return false;
    }
    state_.setVolume(static_cast<uint8_t>(volume));
    M5.Speaker.setVolume(state_.volume());
    return true;
}

bool AudioController::isPlaying() const
{
    return state_.isPlaying();
}

const AudioState& AudioController::getState() const
{
    return state_;
}

bool AudioController::validateWav(const uint8_t* buffer, size_t size) const
{
    if (buffer == nullptr || size < 44) {
        return false;
    }
    if (memcmp(buffer, "RIFF", 4) != 0 || memcmp(buffer + 8, "WAVE", 4) != 0) {
        return false;
    }

    size_t offset = 12;
    bool foundFmt = false;
    bool foundData = false;
    uint16_t audioFormat = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;

    while (offset + 8 <= size) {
        const uint8_t* chunk = buffer + offset;
        const uint32_t chunkSize = readLe32(chunk + 4);
        const size_t dataOffset = offset + 8;
        if (dataOffset + chunkSize > size) {
            return false;
        }

        if (memcmp(chunk, "fmt ", 4) == 0) {
            if (chunkSize < 16) {
                return false;
            }
            audioFormat = readLe16(buffer + dataOffset);
            channels = readLe16(buffer + dataOffset + 2);
            sampleRate = readLe32(buffer + dataOffset + 4);
            bitsPerSample = readLe16(buffer + dataOffset + 14);
            foundFmt = true;
        } else if (memcmp(chunk, "data", 4) == 0) {
            foundData = chunkSize > 0;
        }

        offset = dataOffset + chunkSize + (chunkSize % 2);
    }

    return foundFmt
        && foundData
        && audioFormat == 1
        && channels == 1
        && (sampleRate == 16000 || sampleRate == 24000)
        && bitsPerSample == 16;
}

uint32_t AudioController::extractSampleRate(const uint8_t* buffer, size_t size) const
{
    if (buffer == nullptr || size < 44) {
        return 0;
    }

    size_t offset = 12;
    while (offset + 8 <= size) {
        const uint8_t* chunk = buffer + offset;
        const uint32_t chunkSize = readLe32(chunk + 4);
        const size_t dataOffset = offset + 8;
        if (dataOffset + chunkSize > size) {
            return 0;
        }

        if (memcmp(chunk, "fmt ", 4) == 0 && chunkSize >= 16) {
            return readLe32(buffer + dataOffset + 4);
        }

        offset = dataOffset + chunkSize + (chunkSize % 2);
    }
    return 0;
}

uint32_t AudioController::estimateWavDurationMs(const uint8_t* buffer, size_t size) const
{
    if (buffer == nullptr || size < 44) {
        return 0;
    }

    size_t offset = 12;
    uint32_t sampleRate = 0;
    uint16_t channels = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataSize = 0;

    while (offset + 8 <= size) {
        const uint8_t* chunk = buffer + offset;
        const uint32_t chunkSize = readLe32(chunk + 4);
        const size_t dataOffset = offset + 8;
        if (dataOffset + chunkSize > size) {
            return 0;
        }

        if (memcmp(chunk, "fmt ", 4) == 0 && chunkSize >= 16) {
            channels = readLe16(buffer + dataOffset + 2);
            sampleRate = readLe32(buffer + dataOffset + 4);
            bitsPerSample = readLe16(buffer + dataOffset + 14);
        } else if (memcmp(chunk, "data", 4) == 0) {
            dataSize = chunkSize;
        }

        offset = dataOffset + chunkSize + (chunkSize % 2);
    }

    if (sampleRate == 0 || channels == 0 || bitsPerSample == 0 || dataSize == 0) {
        return 0;
    }

    const uint32_t bytesPerSecond = sampleRate * channels * (bitsPerSample / 8);
    return (dataSize * 1000ULL) / bytesPerSecond;
}

void AudioController::releaseBuffer()
{
    if (wavBuffer_ != nullptr && !state_.isPlaying()) {
        free(wavBuffer_);
        wavBuffer_ = nullptr;
    }
}

}  // namespace stackchan
