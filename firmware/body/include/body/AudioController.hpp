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

    bool beginSpeaker();
    void logHardwareState(const char* label) const;

    bool isPlaying() const;
    const AudioState& getState() const;

private:
    struct ParsedWav {
        size_t dataOffset = 0;
        size_t dataSize = 0;
        uint32_t sampleRate = 0;
        uint16_t channels = 0;
        uint16_t bitsPerSample = 0;
    };

    bool parseWavPcm16Mono(const uint8_t* buffer, size_t size, ParsedWav& out) const;
    int16_t maxAbsFirstSamples(const int16_t* pcm, size_t samples, size_t maxCount) const;
    void releaseBuffer();

    AudioMode mode_ = AudioMode::Idle;
    AudioState state_;
    uint8_t* wavBuffer_ = nullptr;
    uint32_t expectedDurationMs_ = 0;
    uint32_t wavSampleRate_ = 0;
    size_t parsedPcmOffset_ = 0;
    size_t parsedPcmBytes_ = 0;
};

}  // namespace stackchan
