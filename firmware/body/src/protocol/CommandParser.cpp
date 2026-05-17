#include "protocol/CommandParser.hpp"

namespace stackchan::protocol {

ParsedCommand CommandParser::parse(const String& line) const
{
    ParsedCommand command;
    String trimmed = line;
    trimmed.trim();

    const int separator = trimmed.indexOf(':');
    command.name = separator >= 0 ? trimmed.substring(0, separator) : trimmed;
    command.name.trim();
    command.name.toUpperCase();
    command.type = parseCommandType(command.name);

    if (separator < 0) {
        return command;
    }

    int start = separator + 1;
    while (start <= trimmed.length()) {
        if (command.argCount >= kMaxCommandArgs) {
            command.argCount++;
            return command;
        }

        const int next = trimmed.indexOf(':', start);
        String arg = next >= 0 ? trimmed.substring(start, next) : trimmed.substring(start);
        arg.trim();
        command.args[command.argCount] = arg;
        command.argCount++;

        if (next < 0) {
            break;
        }
        start = next + 1;
    }

    return command;
}

CommandType CommandParser::parseCommandType(const String& commandName) const
{
    if (commandName == "PING") {
        return CommandType::Ping;
    }
    if (commandName == "VERSION") {
        return CommandType::Version;
    }
    if (commandName == "HELP") {
        return CommandType::Help;
    }
    if (commandName == "STATUS") {
        return CommandType::Status;
    }
    if (commandName == "FACE") {
        return CommandType::Face;
    }
    if (commandName == "LED") {
        return CommandType::Led;
    }
    if (commandName == "POSE") {
        return CommandType::Pose;
    }
    if (commandName == "MOVE") {
        return CommandType::Move;
    }
    if (commandName == "RESET") {
        return CommandType::Reset;
    }
    if (commandName == "AUDIO") {
        return CommandType::Audio;
    }
    if (commandName == "EVENTS") {
        return CommandType::Events;
    }
    if (commandName == "WIFI") {
        return CommandType::Wifi;
    }

    return CommandType::Unknown;
}

}  // namespace stackchan::protocol
