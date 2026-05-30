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
    mic_.begin();
    wake_.begin();
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
        leavePlaybackMode();
        face_.setSpeaking(false);
        face_.setMouthOpen(false);
        led_.setMood(state_.mood());
    }
    input_.update();
    face_.update();
    led_.update();
    motion_.update();
    wake_.update();
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

bool BodyController::queuePreparedWav(size_t size, String& error)
{
    const bool ok = audio_.queuePlayWav(audio_.preparedBuffer(), size, error);
    if (!ok) {
        return false;
    }

    face_.setSpeaking(true);
    led_.setMood(Mood::Speaking);
    return true;
}

bool BodyController::enterPlaybackMode()
{
    Serial.println("[BODY] enterPlaybackMode");
    logHardwareState("before");

    wakePausedForAudio_ = wake_.getState().running();
    if (wakePausedForAudio_) {
        Serial.println("[BODY] stopping wake");
        wake_.stop();
    }

    const uint32_t deadline = millis() + 1000;
    while (M5.Mic.isRecording() && millis() < deadline) {
        delay(10);
    }
    if (M5.Mic.isRecording()) {
        Serial.println("[BODY][WARN] mic still recording, abort");
        return false;
    }

    mic_.end();
    Serial.println("[BODY] mic ended");

    if (!audio_.beginSpeaker()) {
        Serial.println("[BODY][WARN] speaker begin failed");
        mic_.begin();
        wakePausedForAudio_ = false;
        return false;
    }

    logHardwareState("after");
    return true;
}

void BodyController::leavePlaybackMode()
{
    Serial.println("[BODY] leavePlaybackMode");
    logHardwareState("before");

    M5.Speaker.end();
    delay(50);

    if (wakePausedForAudio_) {
        wakePausedForAudio_ = false;
        mic_.begin();
        String wakeError;
        if (!wake_.start(wakeError)) {
            Serial.print("[BODY][WARN] wake restart failed: ");
            Serial.println(wakeError);
        }
    }

    logHardwareState("after");
}

void BodyController::processAudioQueue()
{
    const AudioState& audioState = audio_.getState();
    if (audioState.isQueued()) {
        String error;
        if (!enterPlaybackMode()) {
            audio_.stop();
            face_.setSpeaking(false);
            face_.setMouthOpen(false);
            led_.setMood(state_.mood());
            return;
        }

        if (!audio_.startQueuedPlay(error)) {
            Serial.print("[BODY][WARN] audio playback start failed: ");
            Serial.println(error);
            leavePlaybackMode();
            face_.setSpeaking(false);
            face_.setMouthOpen(false);
            led_.setMood(state_.mood());
        }
    }
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

bool BodyController::recordMicWav(uint32_t durationMs, String& error)
{
    stopWake();
    stopAudio();
    mic_.begin();
    led_.setMood(Mood::Active);
    const bool ok = mic_.recordWav(durationMs, error);
    led_.setMood(state_.mood());
    return ok;
}

void BodyController::showWakeDetected()
{
    face_.setExpression(Expression::Happy);
    face_.setSpeaking(true);
    led_.setMood(Mood::Warning);
}

void BodyController::clearWakeDetected()
{
    face_.setExpression(state_.expression());
    face_.setSpeaking(false);
    face_.setMouthOpen(false);
    led_.setMood(state_.mood());
}

bool BodyController::startWake(String& error)
{
    stopAudio();
    return wake_.start(error);
}

void BodyController::stopWake()
{
    wake_.stop();
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

const MicState& BodyController::getMicState() const
{
    return mic_.getState();
}

const WakeState& BodyController::getWakeState() const
{
    return wake_.getState();
}

const uint8_t* BodyController::micWavBuffer() const
{
    return mic_.wavBuffer();
}

size_t BodyController::micWavSize() const
{
    return mic_.wavSize();
}

InputController& BodyController::input()
{
    return input_;
}

const InputController& BodyController::input() const
{
    return input_;
}

void BodyController::logHardwareState(const char* label) const
{
    Serial.printf(
        "[BODY] %s: spk enabled=%d running=%d playing=%d | mic enabled=%d running=%d recording=%d\n",
        label,
        M5.Speaker.isEnabled(),
        M5.Speaker.isRunning(),
        M5.Speaker.isPlaying(),
        M5.Mic.isEnabled(),
        M5.Mic.isRunning(),
        M5.Mic.isRecording()
    );
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
