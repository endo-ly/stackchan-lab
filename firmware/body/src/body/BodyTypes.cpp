#include "body/BodyTypes.hpp"

namespace stackchan {

const char* toString(BodyMode mode)
{
    switch (mode) {
        case BodyMode::Booting:
            return "Booting";
        case BodyMode::Idle:
            return "Idle";
        case BodyMode::Ready:
            return "Ready";
        case BodyMode::Error:
            return "Error";
    }

    return "Unknown";
}

const char* toString(Expression expression)
{
    switch (expression) {
        case Expression::Neutral:
            return "Neutral";
        case Expression::Happy:
            return "Happy";
        case Expression::Thinking:
            return "Thinking";
        case Expression::Sleepy:
            return "Sleepy";
        case Expression::Alert:
            return "Alert";
    }

    return "Unknown";
}

const char* toString(Mood mood)
{
    switch (mood) {
        case Mood::Calm:
            return "Calm";
        case Mood::Active:
            return "Active";
        case Mood::Speaking:
            return "Speaking";
        case Mood::Warning:
            return "Warning";
        case Mood::Off:
            return "Off";
    }

    return "Unknown";
}

const char* toString(MotionPose pose)
{
    switch (pose) {
        case MotionPose::Neutral:
            return "Neutral";
        case MotionPose::LookLeft:
            return "LookLeft";
        case MotionPose::LookRight:
            return "LookRight";
        case MotionPose::LookUp:
            return "LookUp";
        case MotionPose::LookDown:
            return "LookDown";
    }

    return "Unknown";
}

}  // namespace stackchan

