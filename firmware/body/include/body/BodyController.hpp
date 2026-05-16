#pragma once

#include "body/BodyState.hpp"
#include "body/AudioController.hpp"
#include "body/DisplayController.hpp"
#include "body/FaceController.hpp"
#include "body/LedController.hpp"
#include "body/MotionController.hpp"

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

    const BodyState& getState() const;
    const FaceState& getFaceState() const;
    const AudioState& getAudioState() const;

private:
    void logState() const;

    BodyState state_;
    DisplayController display_;
    AudioController audio_;
    FaceController face_;
    LedController led_;
    MotionController motion_;
};

}  // namespace stackchan
