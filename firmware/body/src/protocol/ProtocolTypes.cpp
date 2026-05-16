#include "protocol/ProtocolTypes.hpp"

namespace stackchan::protocol {

const char* toString(CommandType type)
{
    switch (type) {
        case CommandType::Ping:
            return "PING";
        case CommandType::Version:
            return "VERSION";
        case CommandType::Help:
            return "HELP";
        case CommandType::Status:
            return "STATUS";
        case CommandType::Face:
            return "FACE";
        case CommandType::Led:
            return "LED";
        case CommandType::Pose:
            return "POSE";
        case CommandType::Move:
            return "MOVE";
        case CommandType::Reset:
            return "RESET";
        case CommandType::Unknown:
            return "UNKNOWN";
    }

    return "UNKNOWN";
}

const char* toString(ProtocolError error)
{
    switch (error) {
        case ProtocolError::UnknownCommand:
            return "UNKNOWN_COMMAND";
        case ProtocolError::InvalidArgument:
            return "INVALID_ARGUMENT";
        case ProtocolError::MissingArgument:
            return "MISSING_ARGUMENT";
        case ProtocolError::TooManyArguments:
            return "TOO_MANY_ARGUMENTS";
        case ProtocolError::CommandTooLong:
            return "COMMAND_TOO_LONG";
        case ProtocolError::InternalError:
            return "INTERNAL_ERROR";
    }

    return "INTERNAL_ERROR";
}

}  // namespace stackchan::protocol
