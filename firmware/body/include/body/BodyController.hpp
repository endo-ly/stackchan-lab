#pragma once

#include "body/BodyState.hpp"
#include "body/AudioController.hpp"
#include "body/DisplayController.hpp"
#include "body/FaceController.hpp"
#include "body/InputController.hpp"
#include "body/LedController.hpp"
#include "body/MicController.hpp"
#include "body/MotionController.hpp"
#include "body/WakeController.hpp"

namespace stackchan {

class BodyController {
public:
    void begin();
    void update();

    void setExpression(Expression expression);
    void setMood(Mood mood);
    void setPose(MotionPose pose);
    void moveTo(int x, int y);
    void showStatus();
    bool prepareWav(size_t size, String& error);
    uint8_t* wavReceiveBuffer();
    bool playPreparedWav(size_t size, String& error);
    void stopAudio();
    bool setAudioVolume(int volume);
    bool recordMicWav(uint32_t durationMs, String& error);
    void showWakeDetected();
    void clearWakeDetected();
    bool startWake(String& error);
    void stopWake();

    const BodyState& getState() const;
    const FaceState& getFaceState() const;
    const AudioState& getAudioState() const;
    const MicState& getMicState() const;
    const WakeState& getWakeState() const;
    const uint8_t* micWavBuffer() const;
    size_t micWavSize() const;
    InputController& input();
    const InputController& input() const;

private:
    void logState() const;

    BodyState state_;
    DisplayController display_;
    AudioController audio_;
    MicController mic_;
    InputController input_;
    FaceController face_;
    LedController led_;
    MotionController motion_;
    WakeController wake_;
};

}  // namespace stackchan
