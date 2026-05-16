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
    M5.Speaker.begin();
    M5.Speaker.setVolume(state_.volume());
    state_.setState(AudioPlaybackState::Idle);
    state_.setPlaying(false);
}

void AudioController::update()
{
    if (state_.isPlaying() && !M5.Speaker.isPlaying()) {
        state_.setPlaying(false);
        state_.setState(AudioPlaybackState::Finished);
        state_.markFinished(millis());
        releaseBuffer();
    }
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

bool AudioController::playWav(uint8_t* buffer, size_t size, String& error)
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

    M5.Speaker.setVolume(state_.volume());
    if (!M5.Speaker.playWav(buffer, size, 1, 0, true)) {
        error = "AUDIO_INVALID_FORMAT";
        state_.setState(AudioPlaybackState::Error);
        state_.setError(error);
        releaseBuffer();
        return false;
    }

    state_.setState(AudioPlaybackState::Playing);
    state_.setPlaying(true);
    state_.markStarted(millis());
    state_.clearError();
    return true;
}

void AudioController::stop()
{
    M5.Speaker.stop();
    state_.setPlaying(false);
    state_.setState(AudioPlaybackState::Idle);
    state_.setCurrentSize(0);
    state_.setReceivedSize(0);
    state_.markFinished(millis());
    releaseBuffer();
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

void AudioController::releaseBuffer()
{
    if (wavBuffer_ != nullptr && !state_.isPlaying()) {
        free(wavBuffer_);
        wavBuffer_ = nullptr;
    }
}

}  // namespace stackchan
