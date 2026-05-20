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
        case CommandType::Audio:
            return "AUDIO";
        case CommandType::Events:
            return "EVENTS";
        case CommandType::Wifi:
            return "WIFI";
        case CommandType::Device:
            return "DEVICE";
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
        case ProtocolError::AudioTooLarge:
            return "AUDIO_TOO_LARGE";
        case ProtocolError::AudioBusy:
            return "AUDIO_BUSY";
        case ProtocolError::AudioReceiveTimeout:
            return "AUDIO_RECEIVE_TIMEOUT";
        case ProtocolError::AudioInvalidFormat:
            return "AUDIO_INVALID_FORMAT";
        case ProtocolError::AudioTransferFailed:
            return "AUDIO_TRANSFER_FAILED";
        case ProtocolError::EventQueueError:
            return "EVENT_QUEUE_ERROR";
        case ProtocolError::UnsupportedInput:
            return "UNSUPPORTED_INPUT";
        case ProtocolError::InternalError:
            return "INTERNAL_ERROR";
    }

    return "INTERNAL_ERROR";
}

}  // namespace stackchan::protocol
