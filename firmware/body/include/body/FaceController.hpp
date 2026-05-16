#pragma once

#include "body/FaceState.hpp"

namespace stackchan {

class FaceController {
public:
    void begin();
    void update();

    void setExpression(Expression expression);
    void setGaze(int x, int y);
    void setMouthOpen(bool open);
    void setSpeaking(bool speaking);
    void blink();
    void render();

    const FaceState& getState() const;

private:
    int clampGaze(int value) const;
    void applyGaze();
    uint32_t nextBlinkDelay() const;

    FaceState state_;
    bool initialized_ = false;
    bool blinkOpenPending_ = false;
    uint32_t blinkStartedAt_ = 0;
    uint32_t lastMouthToggleAt_ = 0;
};

}  // namespace stackchan
