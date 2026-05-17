#pragma once

#include <Arduino.h>

#include "body/BodyController.hpp"
#include "body/BodyTypes.hpp"
#include "protocol/ProtocolTypes.hpp"

namespace stackchan::protocol {

class CommandHandler {
public:
    explicit CommandHandler(BodyController& body);

    String handle(const ParsedCommand& command);
    uint8_t* wavReceiveBuffer();
    String completeWavTransfer(size_t size);

private:
    String handleAudio(const ParsedCommand& command);
    String handleAudioStatus(const ParsedCommand& command) const;
    String handleAudioVolume(const ParsedCommand& command);
    String handleAudioStop(const ParsedCommand& command);
    String handleAudioWav(const ParsedCommand& command);
    String handleEvents(const ParsedCommand& command);
    String handleEventsList(const ParsedCommand& command) const;
    String handleEventsLatest(const ParsedCommand& command) const;
    String handleEventsClear(const ParsedCommand& command);

    String handleFace(const ParsedCommand& command);
    String handleLed(const ParsedCommand& command);
    String handlePose(const ParsedCommand& command);
    String handleMove(const ParsedCommand& command);
    String handleReset(const ParsedCommand& command);
    String handleStatus(const ParsedCommand& command) const;

    bool parseExpression(const String& value, Expression& expression) const;
    bool parseMood(const String& value, Mood& mood) const;
    bool parsePose(const String& value, MotionPose& pose) const;
    bool parseSize(const String& value, size_t& result) const;
    bool parseInteger(const String& value, int& result) const;
    bool hasNoArgs(const ParsedCommand& command) const;

    String missingArgument(const char* name) const;
    String invalidArgument(const char* name, const String& value) const;
    String tooManyArguments(const ParsedCommand& command) const;

    BodyController& body_;
};

}  // namespace stackchan::protocol
