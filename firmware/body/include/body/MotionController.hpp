#pragma once

#include "body/BodyState.hpp"
#include "body/BodyTypes.hpp"

namespace stackchan {

class MotionController {
public:
    void begin(BodyState& state);
    void moveTo(BodyState& state, int x, int y);
    void setPose(BodyState& state, MotionPose pose);
    void reset(BodyState& state);
    void update();

private:
    int clampAxis(const char* axis, int requested, int minValue, int maxValue) const;
};

}  // namespace stackchan

