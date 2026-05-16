#include "body/FaceController.hpp"

#include <Arduino.h>
#include <Avatar.h>

namespace stackchan {
namespace {

constexpr uint32_t kBlinkDurationMs = 140;
constexpr uint32_t kBlinkMinDelayMs = 3000;
constexpr uint32_t kBlinkMaxDelayMs = 7000;
constexpr uint32_t kMouthToggleIntervalMs = 180;

m5avatar::Avatar avatar;

m5avatar::Expression toAvatarExpression(Expression expression)
{
    switch (expression) {
        case Expression::Neutral:
            return m5avatar::Expression::Neutral;
        case Expression::Happy:
            return m5avatar::Expression::Happy;
        case Expression::Sad:
            return m5avatar::Expression::Sad;
        case Expression::Angry:
            return m5avatar::Expression::Angry;
        case Expression::Sleepy:
            return m5avatar::Expression::Sleepy;
        case Expression::Doubt:
            return m5avatar::Expression::Doubt;
    }

    return m5avatar::Expression::Neutral;
}

float gazeToAvatarValue(int value)
{
    return static_cast<float>(value) / 100.0f;
}

}  // namespace

void FaceController::begin()
{
    Serial.println("[FACE] begin");
    avatar.init();
    avatar.setIsAutoBlink(false);

    initialized_ = true;
    state_.setExpression(Expression::Neutral);
    state_.setGaze(0, 0);
    state_.setMouthOpen(false);
    state_.setSpeaking(false);
    state_.setBlinking(false);
    state_.setBlinkTiming(millis(), nextBlinkDelay());
}

void FaceController::update()
{
    if (!initialized_) {
        return;
    }

    const uint32_t now = millis();
    if (state_.isSpeaking() && now - lastMouthToggleAt_ >= kMouthToggleIntervalMs) {
        setMouthOpen(!state_.mouthOpen());
        lastMouthToggleAt_ = now;
    }

    if (blinkOpenPending_ && now - blinkStartedAt_ >= kBlinkDurationMs) {
        avatar.setEyeOpenRatio(1.0f);
        blinkOpenPending_ = false;
        state_.setBlinking(false);
        state_.setBlinkTiming(now, nextBlinkDelay());
        return;
    }

    if (!blinkOpenPending_ && now - state_.lastBlinkAt() >= state_.nextBlinkDelayMs()) {
        blink();
    }
}

void FaceController::setExpression(Expression expression)
{
    state_.setExpression(expression);
    if (!initialized_) {
        return;
    }

    avatar.setExpression(toAvatarExpression(expression));
    if (expression == Expression::Doubt) {
        setGaze(25, -30);
    } else if (expression == Expression::Sleepy) {
        setGaze(0, 20);
    } else {
        setGaze(0, 0);
    }
}

void FaceController::setGaze(int x, int y)
{
    state_.setGaze(clampGaze(x), clampGaze(y));
    applyGaze();
}

void FaceController::setMouthOpen(bool open)
{
    state_.setMouthOpen(open);
    if (initialized_) {
        avatar.setMouthOpenRatio(open ? 1.0f : 0.0f);
    }
}

void FaceController::setSpeaking(bool speaking)
{
    state_.setSpeaking(speaking);
    lastMouthToggleAt_ = millis();
    if (!speaking) {
        setMouthOpen(false);
    }
}

void FaceController::blink()
{
    if (!initialized_) {
        return;
    }

    blinkStartedAt_ = millis();
    blinkOpenPending_ = true;
    state_.setBlinking(true);
    avatar.setEyeOpenRatio(0.0f);
}

void FaceController::render()
{
    // M5Stack-Avatar owns drawing through its internal draw task.
}

const FaceState& FaceController::getState() const
{
    return state_;
}

int FaceController::clampGaze(int value) const
{
    if (value < -100) {
        return -100;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

void FaceController::applyGaze()
{
    if (!initialized_) {
        return;
    }

    const float vertical = gazeToAvatarValue(state_.gazeY());
    const float horizontal = gazeToAvatarValue(state_.gazeX());
    avatar.setLeftGaze(vertical, horizontal);
    avatar.setRightGaze(vertical, horizontal);
}

uint32_t FaceController::nextBlinkDelay() const
{
    return random(kBlinkMinDelayMs, kBlinkMaxDelayMs + 1);
}

}  // namespace stackchan
