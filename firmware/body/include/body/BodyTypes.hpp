#pragma once

namespace stackchan {

enum class BodyMode {
    Booting,
    Idle,
    Ready,
    Error,
};

enum class Expression {
    Neutral,
    Happy,
    Thinking,
    Sleepy,
    Alert,
};

enum class Mood {
    Calm,
    Active,
    Speaking,
    Warning,
    Off,
};

enum class MotionPose {
    Neutral,
    LookLeft,
    LookRight,
    LookUp,
    LookDown,
};

const char* toString(BodyMode mode);
const char* toString(Expression expression);
const char* toString(Mood mood);
const char* toString(MotionPose pose);

}  // namespace stackchan

