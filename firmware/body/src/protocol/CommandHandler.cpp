#include "protocol/CommandHandler.hpp"

#include <ArduinoJson.h>

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

CommandHandler::CommandHandler(BodyController& body, network::NetworkController* network)
    : body_(body)
    , network_(network)
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
                ? "OK HELP commands=PING,VERSION,HELP,STATUS,FACE,LED,POSE,MOVE,RESET,AUDIO,EVENTS,WIFI expressions=neutral,happy,sad,angry,sleepy,doubt moods=calm,active,speaking,warning,off poses=neutral,look_left,look_right,look_up,look_down audio=AUDIO:STATUS,AUDIO:VOLUME,AUDIO:STOP,AUDIO:WAV events=EVENTS,EVENTS:LATEST,EVENTS:CLEAR wifi=WIFI:STATUS,WIFI:SET_JSON,WIFI:CONNECT,WIFI:CLEAR"
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
        case CommandType::Audio:
            return handleAudio(command);
        case CommandType::Events:
            return handleEvents(command);
        case CommandType::Wifi:
            return handleWifi(command);
        case CommandType::Unknown:
            return String("ERR ") + toString(ProtocolError::UnknownCommand) + " command=" + command.name;
    }

    return String("ERR ") + toString(ProtocolError::InternalError);
}

String CommandHandler::handleWifi(const ParsedCommand& command)
{
    if (network_ == nullptr) {
        return "ERR NETWORK_NOT_CONFIGURED";
    }
    if (command.argCount == 0) {
        return missingArgument("wifi_command");
    }
    const String action = normalized(command.args[0]);
    if (action == "status") return handleWifiStatus(command);
    if (action == "set_json") return handleWifiSetJson(command);
    if (action == "connect") return handleWifiConnect(command);
    if (action == "clear") return handleWifiClear(command);
    return invalidArgument("wifi_command", command.args[0]);
}

String CommandHandler::handleWifiStatus(const ParsedCommand& command) const
{
    if (command.argCount > 1) return tooManyArguments(command);
    const network::NetworkState& state = network_->getState();
    String response = "OK WIFI STATUS connected=";
    response += state.connected ? "true" : "false";
    response += " ssid=";
    response += state.ssid;
    response += " ip=";
    response += state.ip;
    response += " hostname=";
    response += state.hostname;
    response += " rssi=";
    response += state.rssi;
    response += " auth=";
    response += state.authEnabled ? "true" : "false";
    return response;
}

String CommandHandler::handleWifiSetJson(const ParsedCommand& command)
{
    if (command.argCount < 2) return missingArgument("size");
    if (command.argCount > 2) return tooManyArguments(command);
    size_t size = 0;
    if (!parseSize(command.args[1], size) || size == 0 || size > 1024) {
        return invalidArgument("size", command.args[1]);
    }
    String response = "READY WIFI JSON size=";
    response += size;
    return response;
}

String CommandHandler::handleWifiConnect(const ParsedCommand& command)
{
    if (command.argCount > 1) return tooManyArguments(command);
    if (!network_->connect()) return "ERR WIFI_CONNECT_FAILED reason=timeout";
    const network::NetworkState& state = network_->getState();
    return String("OK WIFI CONNECTED ip=") + state.ip + " hostname=" + state.hostname;
}

String CommandHandler::handleWifiClear(const ParsedCommand& command)
{
    if (command.argCount > 1) return tooManyArguments(command);
    if (!network_->clearConfig()) return "ERR WIFI_CONFIG_CLEAR_FAILED";
    return "OK WIFI CLEAR";
}

String CommandHandler::completeWifiJsonTransfer(const String& json)
{
    if (network_ == nullptr) {
        return "ERR NETWORK_NOT_CONFIGURED";
    }
    JsonDocument doc;
    if (deserializeJson(doc, json)) {
        return "ERR WIFI_JSON_INVALID";
    }
    network::WiFiConfig config;
    config.ssid = doc["ssid"] | "";
    config.password = doc["password"] | "";
    config.hostname = doc["hostname"] | "stackchan-001";
    config.authToken = doc["authToken"] | "";
    if (config.ssid.length() == 0 || config.hostname.length() == 0) {
        return "ERR WIFI_JSON_INVALID";
    }
    if (!network_->wifi().saveConfig(config)) {
        return "ERR WIFI_CONFIG_SAVE_FAILED";
    }
    if (!network_->connect(config)) {
        return "ERR WIFI_CONNECT_FAILED";
    }
    return String("OK WIFI SET ssid=") + config.ssid + " hostname=" + config.hostname;
}

String CommandHandler::handleEvents(const ParsedCommand& command)
{
    if (command.argCount == 0) {
        return handleEventsList(command);
    }

    const String action = normalized(command.args[0]);
    if (action == "latest") {
        return handleEventsLatest(command);
    }
    if (action == "clear") {
        return handleEventsClear(command);
    }

    return invalidArgument("events_command", command.args[0]);
}

String CommandHandler::handleEventsList(const ParsedCommand& command) const
{
    if (!hasNoArgs(command)) {
        return tooManyArguments(command);
    }

    const InputController& input = body_.input();
    String response = "OK EVENTS count=";
    response += input.eventCount();
    if (!input.hasEvents()) {
        return response;
    }

    for (size_t index = 0; index < input.eventCount(); ++index) {
        InputEvent event;
        if (!input.getEvent(index, event)) {
            return String("ERR ") + toString(ProtocolError::EventQueueError);
        }
        response += "\n";
        response += formatInputEvent(event);
    }
    response += "\nEND EVENTS";
    return response;
}

String CommandHandler::handleEventsLatest(const ParsedCommand& command) const
{
    if (command.argCount > 1) {
        return tooManyArguments(command);
    }

    InputEvent event;
    if (!body_.input().getLatestEvent(event)) {
        return "OK EVENTS LATEST none";
    }

    String response = "OK EVENTS LATEST id=";
    response += event.id;
    response += " type=";
    response += event.type;
    response += " target=";
    response += event.target;
    response += " value=";
    response += event.value;
    response += " timestamp=";
    response += event.timestamp;
    return response;
}

String CommandHandler::handleEventsClear(const ParsedCommand& command)
{
    if (command.argCount > 1) {
        return tooManyArguments(command);
    }

    body_.input().clearEvents();
    return "OK EVENTS CLEAR";
}

uint8_t* CommandHandler::wavReceiveBuffer()
{
    return body_.wavReceiveBuffer();
}

String CommandHandler::completeWavTransfer(size_t size)
{
    String error;
    if (!body_.playPreparedWav(size, error)) {
        return String("ERR ") + error;
    }
    String response = "OK AUDIO PLAY size=";
    response += size;
    return response;
}

String CommandHandler::handleAudio(const ParsedCommand& command)
{
    if (command.argCount == 0) {
        return missingArgument("audio_command");
    }

    const String action = normalized(command.args[0]);
    if (action == "status") {
        return handleAudioStatus(command);
    }
    if (action == "volume") {
        return handleAudioVolume(command);
    }
    if (action == "stop") {
        return handleAudioStop(command);
    }
    if (action == "wav") {
        return handleAudioWav(command);
    }

    return invalidArgument("audio_command", command.args[0]);
}

String CommandHandler::handleAudioStatus(const ParsedCommand& command) const
{
    if (command.argCount > 1) {
        return tooManyArguments(command);
    }

    const AudioState& audio = body_.getAudioState();
    String response = "OK AUDIO STATUS state=";
    response += toString(audio.state());
    response += " playing=";
    response += audio.isPlaying() ? "true" : "false";
    response += " volume=";
    response += audio.volume();
    response += " size=";
    response += audio.currentSize();
    response += " received=";
    response += audio.receivedSize();
    return response;
}

String CommandHandler::handleAudioVolume(const ParsedCommand& command)
{
    if (command.argCount < 2) {
        return missingArgument("volume");
    }
    if (command.argCount > 2) {
        return tooManyArguments(command);
    }

    int volume = 0;
    if (!parseInteger(command.args[1], volume) || !body_.setAudioVolume(volume)) {
        return invalidArgument("volume", command.args[1]);
    }

    String response = "OK AUDIO VOLUME ";
    response += volume;
    return response;
}

String CommandHandler::handleAudioStop(const ParsedCommand& command)
{
    if (command.argCount > 1) {
        return tooManyArguments(command);
    }

    body_.stopAudio();
    return "OK AUDIO STOP";
}

String CommandHandler::handleAudioWav(const ParsedCommand& command)
{
    if (command.argCount < 2) {
        return missingArgument("size");
    }
    if (command.argCount > 2) {
        return tooManyArguments(command);
    }

    size_t size = 0;
    if (!parseSize(command.args[1], size)) {
        return invalidArgument("size", command.args[1]);
    }

    String error;
    if (!body_.prepareWav(size, error)) {
        if (error == "AUDIO_TOO_LARGE") {
            return String("ERR AUDIO_TOO_LARGE max=") + kMaxWavBytes;
        }
        return String("ERR ") + error;
    }

    String response = "READY AUDIO WAV size=";
    response += size;
    return response;
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
    body_.stopAudio();
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
    const InputState& input = body_.input().getState();
    response += " eventCount=";
    response += body_.input().eventCount();
    response += " latestEvent=";
    response += input.latestType();
    response += " latestTarget=";
    response += input.latestTarget();
    response += " latestValue=";
    response += input.latestValue();
    response += " touchActive=";
    response += input.touchActive() ? "true" : "false";
    response += " buttonActive=";
    response += input.buttonActive() ? "true" : "false";
    response += " imuMoving=";
    response += input.imuMoving() ? "true" : "false";
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

bool CommandHandler::parseSize(const String& value, size_t& result) const
{
    int parsed = 0;
    if (!parseInteger(value, parsed) || parsed < 0) {
        return false;
    }
    result = static_cast<size_t>(parsed);
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
