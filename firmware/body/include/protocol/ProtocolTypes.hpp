#pragma once

#include <Arduino.h>

namespace stackchan::protocol {

constexpr const char* kFirmwareVersion = "0.4.0";
constexpr const char* kProtocolVersion = "0.1.0";
constexpr const char* kBoardName = "stackchan-cores3";
constexpr size_t kMaxCommandLength = 128;
constexpr size_t kMaxCommandArgs = 3;

enum class CommandType {
    Ping,
    Version,
    Help,
    Status,
    Face,
    Led,
    Pose,
    Move,
    Reset,
    Unknown,
};

enum class CommandResult {
    Ok,
    Error,
};

enum class ProtocolError {
    UnknownCommand,
    InvalidArgument,
    MissingArgument,
    TooManyArguments,
    CommandTooLong,
    InternalError,
};

struct ParsedCommand {
    CommandType type = CommandType::Unknown;
    String name;
    String args[kMaxCommandArgs];
    size_t argCount = 0;
};

const char* toString(CommandType type);
const char* toString(ProtocolError error);

}  // namespace stackchan::protocol
