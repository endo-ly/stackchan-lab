#include "body/BodyController.hpp"

#include <Arduino.h>
#include <M5StackChan.h>

namespace stackchan {
namespace {

constexpr uint32_t kDemoDelayMs = 2500;

}  // namespace

void BodyController::begin()
{
    Serial.println("[BOOT] Initializing body controller");

    state_.setMode(BodyMode::Booting);
    state_.setExpression(Expression::Neutral);
    state_.setMood(Mood::Off);
    state_.setPose(MotionPose::Neutral);

    M5StackChan.begin();

    display_.begin();
    led_.begin();
    motion_.begin(state_);

    display_.showBoot();
    led_.setMood(Mood::Calm);
    state_.setMood(Mood::Calm);

    state_.setMode(BodyMode::Idle);
    display_.showReady();
    readyAt_ = millis();

    logState();
    Serial.println("[BOOT] Ready");
}

void BodyController::update()
{
    M5StackChan.update();
    display_.update();
    led_.update();
    motion_.update();
    runDemoOnce();
}

void BodyController::setExpression(Expression expression)
{
    state_.setExpression(expression);
    display_.showExpression(expression);

    Serial.print("[BODY] expression=");
    Serial.println(toString(expression));
}

void BodyController::setMood(Mood mood)
{
    state_.setMood(mood);
    led_.setMood(mood);

    Serial.print("[BODY] mood=");
    Serial.println(toString(mood));
}

void BodyController::setPose(MotionPose pose)
{
    motion_.setPose(state_, pose);

    Serial.print("[BODY] pose=");
    Serial.println(toString(pose));
}

void BodyController::moveTo(int x, int y)
{
    motion_.moveTo(state_, x, y);

    Serial.print("[BODY] servo x=");
    Serial.print(state_.servoX());
    Serial.print(" y=");
    Serial.println(state_.servoY());
}

void BodyController::showStatus()
{
    display_.showStatus(state_);
}

const BodyState& BodyController::getState() const
{
    return state_;
}

void BodyController::runDemoOnce()
{
    if (demoDone_ || readyAt_ == 0) {
        return;
    }

    if (millis() - readyAt_ < kDemoDelayMs) {
        return;
    }

    Serial.println("[BODY] run simple demo");
    setExpression(Expression::Thinking);
    setMood(Mood::Active);
    setPose(MotionPose::Neutral);
    demoDone_ = true;
}

void BodyController::logState() const
{
    Serial.print("[BODY] mode=");
    Serial.print(toString(state_.mode()));
    Serial.print(" expression=");
    Serial.print(toString(state_.expression()));
    Serial.print(" mood=");
    Serial.print(toString(state_.mood()));
    Serial.print(" pose=");
    Serial.println(toString(state_.pose()));
}

}  // namespace stackchan

