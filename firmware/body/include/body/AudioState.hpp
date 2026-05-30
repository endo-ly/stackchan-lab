#pragma once

#include <Arduino.h>

namespace stackchan {

enum class AudioPlaybackState {
    Idle,
    Receiving,
    Queued,
    Playing,
    Finished,
    Error,
};

class AudioState {
public:
    AudioPlaybackState state() const;
    uint8_t volume() const;
    bool isPlaying() const;
    size_t currentSize() const;
    size_t receivedSize() const;
    uint32_t startedAt() const;
    uint32_t finishedAt() const;
    const String& lastError() const;

    void setState(AudioPlaybackState state);
    void setVolume(uint8_t volume);
    void setQueued(bool queued);
    bool isQueued() const;
    void setPlaying(bool playing);
    void setCurrentSize(size_t size);
    void setReceivedSize(size_t size);
    void markStarted(uint32_t now);
    void markFinished(uint32_t now);
    void setError(const String& error);
    void clearError();

private:
    AudioPlaybackState state_ = AudioPlaybackState::Idle;
    uint8_t volume_ = 180;
    bool isPlaying_ = false;
    bool isQueued_ = false;
    size_t currentSize_ = 0;
    size_t receivedSize_ = 0;
    uint32_t startedAt_ = 0;
    uint32_t finishedAt_ = 0;
    String lastError_;
};

const char* toString(AudioPlaybackState state);

}  // namespace stackchan
