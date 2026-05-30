#include "body/AudioState.hpp"

namespace stackchan {

AudioPlaybackState AudioState::state() const { return state_; }
uint8_t AudioState::volume() const { return volume_; }
bool AudioState::isPlaying() const { return isPlaying_; }
size_t AudioState::currentSize() const { return currentSize_; }
size_t AudioState::receivedSize() const { return receivedSize_; }
uint32_t AudioState::startedAt() const { return startedAt_; }
uint32_t AudioState::finishedAt() const { return finishedAt_; }
const String& AudioState::lastError() const { return lastError_; }

void AudioState::setState(AudioPlaybackState state) { state_ = state; }
void AudioState::setVolume(uint8_t volume) { volume_ = volume; }
void AudioState::setPlaying(bool playing) { isPlaying_ = playing; }
void AudioState::setCurrentSize(size_t size) { currentSize_ = size; }
void AudioState::setQueued(bool queued) { isQueued_ = queued; }
bool AudioState::isQueued() const { return isQueued_; }
void AudioState::setReceivedSize(size_t size) { receivedSize_ = size; }
void AudioState::markStarted(uint32_t now) { startedAt_ = now; }
void AudioState::markFinished(uint32_t now) { finishedAt_ = now; }
void AudioState::setError(const String& error) { lastError_ = error; }
void AudioState::clearError() { lastError_ = ""; }

const char* toString(AudioPlaybackState state)
{
    switch (state) {
        case AudioPlaybackState::Idle:
            return "Idle";
        case AudioPlaybackState::Receiving:
            return "Receiving";
        case AudioPlaybackState::Queued:
            return "Queued";
        case AudioPlaybackState::Playing:
            return "Playing";
        case AudioPlaybackState::Finished:
            return "Finished";
        case AudioPlaybackState::Error:
            return "Error";
    }

    return "Error";
}

}  // namespace stackchan
