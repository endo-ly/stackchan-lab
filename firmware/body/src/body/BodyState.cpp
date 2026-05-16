#include "body/BodyState.hpp"

namespace stackchan {

BodyMode BodyState::mode() const
{
    return mode_;
}

Expression BodyState::expression() const
{
    return expression_;
}

Mood BodyState::mood() const
{
    return mood_;
}

MotionPose BodyState::pose() const
{
    return pose_;
}

int BodyState::servoX() const
{
    return servoX_;
}

int BodyState::servoY() const
{
    return servoY_;
}

const char* BodyState::lastError() const
{
    return lastError_;
}

uint32_t BodyState::updatedAt() const
{
    return updatedAt_;
}

void BodyState::setMode(BodyMode mode)
{
    mode_ = mode;
    touch();
}

void BodyState::setExpression(Expression expression)
{
    expression_ = expression;
    touch();
}

void BodyState::setMood(Mood mood)
{
    mood_ = mood;
    touch();
}

void BodyState::setPose(MotionPose pose)
{
    pose_ = pose;
    touch();
}

void BodyState::setServoPosition(int x, int y)
{
    servoX_ = x;
    servoY_ = y;
    touch();
}

void BodyState::setError(const char* message)
{
    mode_ = BodyMode::Error;
    lastError_ = message;
    touch();
}

void BodyState::clearError()
{
    lastError_ = "";
    touch();
}

void BodyState::touch()
{
    updatedAt_ = millis();
}

}  // namespace stackchan

