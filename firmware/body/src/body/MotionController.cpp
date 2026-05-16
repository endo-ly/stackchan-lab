#include "body/MotionController.hpp"

#include <M5StackChan.h>

namespace stackchan {
namespace {

constexpr int kSafeXMin = -15;
constexpr int kSafeXMax = 15;
constexpr int kSafeYMin = -10;
constexpr int kSafeYMax = 10;
constexpr int kNeutralX = 0;
constexpr int kNeutralY = 0;
constexpr int kMoveSpeed = 200;

}  // namespace

void MotionController::begin(BodyState& state)
{
    Serial.println("[MOTION] begin");
    M5StackChan.setServoPowerEnabled(true);
    reset(state);
}

void MotionController::moveTo(BodyState& state, int x, int y)
{
    const int appliedX = clampAxis("x", x, kSafeXMin, kSafeXMax);
    const int appliedY = clampAxis("y", y, kSafeYMin, kSafeYMax);

    Serial.print("[MOTION] move x=");
    Serial.print(appliedX);
    Serial.print(" y=");
    Serial.println(appliedY);

    M5StackChan.Motion.move(appliedX, appliedY, kMoveSpeed);
    state.setServoPosition(appliedX, appliedY);
}

void MotionController::setPose(BodyState& state, MotionPose pose)
{
    Serial.print("[MOTION] pose=");
    Serial.println(toString(pose));

    state.setPose(pose);

    switch (pose) {
        case MotionPose::Neutral:
            moveTo(state, kNeutralX, kNeutralY);
            break;
        case MotionPose::LookLeft:
            moveTo(state, -8, 0);
            break;
        case MotionPose::LookRight:
            moveTo(state, 8, 0);
            break;
        case MotionPose::LookUp:
            moveTo(state, 0, 6);
            break;
        case MotionPose::LookDown:
            moveTo(state, 0, -6);
            break;
    }
}

void MotionController::reset(BodyState& state)
{
    Serial.println("[MOTION] reset to neutral");
    M5StackChan.Motion.goHome(kMoveSpeed);
    state.setPose(MotionPose::Neutral);
    state.setServoPosition(kNeutralX, kNeutralY);
}

void MotionController::update()
{
}

int MotionController::clampAxis(const char* axis, int requested, int minValue, int maxValue) const
{
    int applied = requested;

    if (applied < minValue) {
        applied = minValue;
    }

    if (applied > maxValue) {
        applied = maxValue;
    }

    if (applied != requested) {
        Serial.print("[MOTION][WARN] ");
        Serial.print(axis);
        Serial.print(" value clamped: requested=");
        Serial.print(requested);
        Serial.print(", applied=");
        Serial.println(applied);
    }

    return applied;
}

}  // namespace stackchan

