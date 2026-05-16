#pragma once

#include "body/BodyState.hpp"
#include "body/DisplayController.hpp"
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

    const BodyState& getState() const;

private:
    void logState() const;

    BodyState state_;
    DisplayController display_;
    LedController led_;
    MotionController motion_;
};

}  // namespace stackchan
