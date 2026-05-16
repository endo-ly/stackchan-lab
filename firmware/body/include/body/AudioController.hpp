#pragma once

#include <Arduino.h>

#include "body/AudioState.hpp"

namespace stackchan {

constexpr size_t kMaxWavBytes = 1048576;

class AudioController {
public:
    void begin();
    void update();

    bool prepareWav(size_t size, String& error);
    uint8_t* preparedBuffer();
    bool playWav(uint8_t* buffer, size_t size, String& error);
    void stop();
    bool setVolume(int volume);

    bool isPlaying() const;
    const AudioState& getState() const;

private:
    bool validateWav(const uint8_t* buffer, size_t size) const;
    void releaseBuffer();

    AudioState state_;
    uint8_t* wavBuffer_ = nullptr;
};

}  // namespace stackchan
