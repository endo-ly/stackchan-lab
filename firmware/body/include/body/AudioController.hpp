#pragma once

#include <Arduino.h>

#include "body/AudioState.hpp"

namespace stackchan {

constexpr size_t kMaxWavBytes = 1048576;

enum class AudioMode {
    Idle,
    Capture,
    Playback,
};

class AudioController {
public:
    void begin();
    void update();

    AudioMode mode() const;
    void setMode(AudioMode mode);

    bool prepareWav(size_t size, String& error);
    uint8_t* preparedBuffer();
    bool queuePlayWav(uint8_t* buffer, size_t size, String& error);
    bool startQueuedPlay(String& error);
    void stop();
    bool setVolume(int volume);

    bool rebeginSpeakerForWav(uint32_t sampleRate);
    void logHardwareState(const char* label) const;
    uint32_t extractSampleRate(const uint8_t* buffer, size_t size) const;

    uint32_t preparedSampleRate() const;

    bool isPlaying() const;
    const AudioState& getState() const;

private:
    bool validateWav(const uint8_t* buffer, size_t size) const;
    uint32_t estimateWavDurationMs(const uint8_t* buffer, size_t size) const;
    void releaseBuffer();

    AudioMode mode_ = AudioMode::Idle;
    AudioState state_;
    uint8_t* wavBuffer_ = nullptr;
    uint32_t expectedDurationMs_ = 0;
    uint32_t wavSampleRate_ = 0;
};

}  // namespace stackchan
