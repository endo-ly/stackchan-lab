#pragma once

#include <Arduino.h>

#include "protocol/ProtocolTypes.hpp"

namespace stackchan::protocol {

class CommandParser {
public:
    ParsedCommand parse(const String& line) const;

private:
    CommandType parseCommandType(const String& commandName) const;
};

}  // namespace stackchan::protocol
