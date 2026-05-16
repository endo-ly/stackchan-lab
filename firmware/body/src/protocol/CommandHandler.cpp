#include "protocol/CommandHandler.hpp"

namespace stackchan::protocol {
namespace {

String normalized(String value)
{
    value.trim();
    value.toLowerCase();
    return value;
}

String okWithValue(const char* command, const String& value)
{
    String response = "OK ";
    response += command;
    response += " ";
    response += value;
    return response;
}

}  // namespace

CommandHandler::CommandHandler(BodyController& body)
    : body_(body)
{
}

String CommandHandler::handle(const ParsedCommand& command)
{
    switch (command.type) {
        case CommandType::Ping:
            return hasNoArgs(command) ? "OK PONG" : tooManyArguments(command);
        case CommandType::Version:
            if (!hasNoArgs(command)) {
                return tooManyArguments(command);
            }
            return String("OK VERSION firmware=") + kFirmwareVersion
                + " protocol=" + kProtocolVersion
                + " board=" + kBoardName;
        case CommandType::Help:
            return hasNoArgs(command)
                ? "OK HELP commands=PING,VERSION,HELP,STATUS,FACE,LED,POSE,MOVE,RESET expressions=neutral,happy,sad,angry,sleepy,doubt moods=calm,active,speaking,warning,off poses=neutral,look_left,look_right,look_up,look_down"
                : tooManyArguments(command);
        case CommandType::Status:
            return handleStatus(command);
        case CommandType::Face:
            return handleFace(command);
        case CommandType::Led:
            return handleLed(command);
        case CommandType::Pose:
            return handlePose(command);
        case CommandType::Move:
            return handleMove(command);
        case CommandType::Reset:
            return handleReset(command);
        case CommandType::Unknown:
            return String("ERR ") + toString(ProtocolError::UnknownCommand) + " command=" + command.name;
    }

    return String("ERR ") + toString(ProtocolError::InternalError);
}

String CommandHandler::handleFace(const ParsedCommand& command)
{
    if (command.argCount == 0) {
        return missingArgument("expression");
    }
    if (command.argCount > 1) {
        return tooManyArguments(command);
    }

    Expression expression;
    const String value = normalized(command.args[0]);
    if (!parseExpression(value, expression)) {
        return invalidArgument("expression", command.args[0]);
    }

    body_.setExpression(expression);
    return okWithValue("FACE", value);
}

String CommandHandler::handleLed(const ParsedCommand& command)
{
    if (command.argCount == 0) {
        return missingArgument("mood");
    }
    if (command.argCount > 1) {
        return tooManyArguments(command);
    }

    Mood mood;
    const String value = normalized(command.args[0]);
    if (!parseMood(value, mood)) {
        return invalidArgument("mood", command.args[0]);
    }

    body_.setMood(mood);
    return okWithValue("LED", value);
}

String CommandHandler::handlePose(const ParsedCommand& command)
{
    if (command.argCount == 0) {
        return missingArgument("pose");
    }
    if (command.argCount > 1) {
        return tooManyArguments(command);
    }

    MotionPose pose;
    const String value = normalized(command.args[0]);
    if (!parsePose(value, pose)) {
        return invalidArgument("pose", command.args[0]);
    }

    body_.setPose(pose);
    return okWithValue("POSE", value);
}

String CommandHandler::handleMove(const ParsedCommand& command)
{
    if (command.argCount < 2) {
        return missingArgument("x,y");
    }
    if (command.argCount > 2) {
        return tooManyArguments(command);
    }

    int x = 0;
    int y = 0;
    if (!parseInteger(command.args[0], x)) {
        return invalidArgument("x", command.args[0]);
    }
    if (!parseInteger(command.args[1], y)) {
        return invalidArgument("y", command.args[1]);
    }

    body_.moveTo(x, y);
    const BodyState& state = body_.getState();
    const bool clamped = state.servoX() != x || state.servoY() != y;

    String response = "OK MOVE x=";
    response += state.servoX();
    response += " y=";
    response += state.servoY();
    if (clamped) {
        response += " clamped=true";
    }
    return response;
}

String CommandHandler::handleReset(const ParsedCommand& command)
{
    if (!hasNoArgs(command)) {
        return tooManyArguments(command);
    }

    body_.setExpression(Expression::Neutral);
    body_.setMood(Mood::Calm);
    body_.setPose(MotionPose::Neutral);
    body_.showStatus();
    return "OK RESET";
}

String CommandHandler::handleStatus(const ParsedCommand& command) const
{
    if (!hasNoArgs(command)) {
        return tooManyArguments(command);
    }

    const BodyState& state = body_.getState();
    String response = "OK STATUS mode=";
    response += toString(state.mode());
    response += " expression=";
    response += toString(state.expression());
    response += " mood=";
    response += toString(state.mood());
    response += " pose=";
    response += toString(state.pose());
    response += " x=";
    response += state.servoX();
    response += " y=";
    response += state.servoY();
    const FaceState& face = body_.getFaceState();
    response += " gazeX=";
    response += face.gazeX();
    response += " gazeY=";
    response += face.gazeY();
    response += " speaking=";
    response += face.isSpeaking() ? "true" : "false";
    response += " blinking=";
    response += face.isBlinking() ? "true" : "false";
    return response;
}

bool CommandHandler::parseExpression(const String& value, Expression& expression) const
{
    if (value == "neutral") {
        expression = Expression::Neutral;
        return true;
    }
    if (value == "happy") {
        expression = Expression::Happy;
        return true;
    }
    if (value == "sad") {
        expression = Expression::Sad;
        return true;
    }
    if (value == "angry") {
        expression = Expression::Angry;
        return true;
    }
    if (value == "sleepy") {
        expression = Expression::Sleepy;
        return true;
    }
    if (value == "doubt") {
        expression = Expression::Doubt;
        return true;
    }
    return false;
}

bool CommandHandler::parseMood(const String& value, Mood& mood) const
{
    if (value == "calm") {
        mood = Mood::Calm;
        return true;
    }
    if (value == "active") {
        mood = Mood::Active;
        return true;
    }
    if (value == "speaking") {
        mood = Mood::Speaking;
        return true;
    }
    if (value == "warning") {
        mood = Mood::Warning;
        return true;
    }
    if (value == "off") {
        mood = Mood::Off;
        return true;
    }
    return false;
}

bool CommandHandler::parsePose(const String& value, MotionPose& pose) const
{
    if (value == "neutral") {
        pose = MotionPose::Neutral;
        return true;
    }
    if (value == "look_left") {
        pose = MotionPose::LookLeft;
        return true;
    }
    if (value == "look_right") {
        pose = MotionPose::LookRight;
        return true;
    }
    if (value == "look_up") {
        pose = MotionPose::LookUp;
        return true;
    }
    if (value == "look_down") {
        pose = MotionPose::LookDown;
        return true;
    }
    return false;
}

bool CommandHandler::parseInteger(const String& value, int& result) const
{
    String trimmed = value;
    trimmed.trim();
    if (trimmed.length() == 0) {
        return false;
    }

    int start = 0;
    if (trimmed[0] == '-' || trimmed[0] == '+') {
        if (trimmed.length() == 1) {
            return false;
        }
        start = 1;
    }

    for (int i = start; i < trimmed.length(); ++i) {
        if (!isDigit(trimmed[i])) {
            return false;
        }
    }

    result = trimmed.toInt();
    return true;
}

bool CommandHandler::hasNoArgs(const ParsedCommand& command) const
{
    return command.argCount == 0;
}

String CommandHandler::missingArgument(const char* name) const
{
    return String("ERR ") + toString(ProtocolError::MissingArgument) + " " + name;
}

String CommandHandler::invalidArgument(const char* name, const String& value) const
{
    return String("ERR ") + toString(ProtocolError::InvalidArgument) + " " + name + "=" + value;
}

String CommandHandler::tooManyArguments(const ParsedCommand& command) const
{
    return String("ERR ") + toString(ProtocolError::TooManyArguments) + " command=" + toString(command.type);
}

}  // namespace stackchan::protocol
