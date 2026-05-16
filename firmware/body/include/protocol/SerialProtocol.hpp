#pragma once

#include <Arduino.h>

#include "body/BodyController.hpp"
#include "protocol/CommandHandler.hpp"
#include "protocol/CommandParser.hpp"

namespace stackchan::protocol {

class SerialProtocol {
public:
    explicit SerialProtocol(BodyController& body);

    void begin();
    void update();
    void processLine(const String& line);

private:
    bool maybeReceiveWav(const String& response);
    bool parseReadyWavSize(const String& response, size_t& size) const;
    void handleChar(char value);
    void writeResponse(const String& response);
    void resetInput();

    CommandParser parser_;
    CommandHandler handler_;
    String input_;
    bool inputTooLong_ = false;
};

}  // namespace stackchan::protocol
