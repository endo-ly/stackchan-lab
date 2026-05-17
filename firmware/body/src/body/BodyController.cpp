#include "body/BodyController.hpp"

#include <Arduino.h>
#include <M5StackChan.h>

namespace stackchan {

void BodyController::begin()
{
    Serial.println("[BOOT] Initializing body controller");

    state_.setMode(BodyMode::Booting);
    state_.setExpression(Expression::Neutral);
    state_.setMood(Mood::Off);
    state_.setPose(MotionPose::Neutral);

    M5StackChan.begin();

    display_.begin();
    display_.showBoot();
    audio_.begin();
    input_.begin();
    led_.begin();
    motion_.begin(state_);

    led_.setMood(Mood::Calm);
    state_.setMood(Mood::Calm);
    face_.begin();
    state_.setExpression(Expression::Neutral);

    state_.setMode(BodyMode::Ready);

    logState();
    Serial.println("[BOOT] Ready");
}

void BodyController::update()
{
    M5StackChan.update();
    display_.update();
    const bool wasAudioPlaying = audio_.isPlaying();
    audio_.update();
    if (wasAudioPlaying && !audio_.isPlaying()) {
        face_.setSpeaking(false);
        face_.setMouthOpen(false);
        led_.setMood(state_.mood());
    }
    input_.update();
    face_.update();
    led_.update();
    motion_.update();
}

void BodyController::setExpression(Expression expression)
{
    state_.setExpression(expression);
    face_.setExpression(expression);

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

bool BodyController::prepareWav(size_t size, String& error)
{
    return audio_.prepareWav(size, error);
}

uint8_t* BodyController::wavReceiveBuffer()
{
    return audio_.preparedBuffer();
}

bool BodyController::playPreparedWav(size_t size, String& error)
{
    const bool ok = audio_.playWav(audio_.preparedBuffer(), size, error);
    if (!ok) {
        return false;
    }

    face_.setSpeaking(true);
    led_.setMood(Mood::Speaking);
    return true;
}

void BodyController::stopAudio()
{
    audio_.stop();
    face_.setSpeaking(false);
    face_.setMouthOpen(false);
    led_.setMood(state_.mood());
}

bool BodyController::setAudioVolume(int volume)
{
    return audio_.setVolume(volume);
}

const BodyState& BodyController::getState() const
{
    return state_;
}

const FaceState& BodyController::getFaceState() const
{
    return face_.getState();
}

const AudioState& BodyController::getAudioState() const
{
    return audio_.getState();
}

InputController& BodyController::input()
{
    return input_;
}

const InputController& BodyController::input() const
{
    return input_;
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
