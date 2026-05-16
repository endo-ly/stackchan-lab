#pragma once

#include <Arduino.h>

#include "body/BodyTypes.hpp"

namespace stackchan {

class BodyState {
public:
    BodyMode mode() const;
    Expression expression() const;
    Mood mood() const;
    MotionPose pose() const;
    int servoX() const;
    int servoY() const;
    const char* lastError() const;
    uint32_t updatedAt() const;

    void setMode(BodyMode mode);
    void setExpression(Expression expression);
    void setMood(Mood mood);
    void setPose(MotionPose pose);
    void setServoPosition(int x, int y);
    void setError(const char* message);
    void clearError();
    void touch();

private:
    BodyMode mode_ = BodyMode::Booting;
    Expression expression_ = Expression::Neutral;
    Mood mood_ = Mood::Off;
    MotionPose pose_ = MotionPose::Neutral;
    int servoX_ = 0;
    int servoY_ = 0;
    const char* lastError_ = "";
    uint32_t updatedAt_ = 0;
};

}  // namespace stackchan

