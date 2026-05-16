#include "protocol/SerialProtocol.hpp"

namespace stackchan::protocol {

SerialProtocol::SerialProtocol(BodyController& body)
    : handler_(body)
{
}

void SerialProtocol::begin()
{
    input_.reserve(kMaxCommandLength);
    Serial.println("[PROTO] Serial protocol ready");
}

void SerialProtocol::update()
{
    while (Serial.available() > 0) {
        handleChar(static_cast<char>(Serial.read()));
    }
}

void SerialProtocol::processLine(const String& line)
{
    String trimmed = line;
    trimmed.trim();
    if (trimmed.length() == 0) {
        return;
    }

    Serial.print("[PROTO] received: ");
    Serial.println(trimmed);

    ParsedCommand command = parser_.parse(trimmed);
    writeResponse(handler_.handle(command));
}

void SerialProtocol::handleChar(char value)
{
    if (value == '\r') {
        return;
    }

    if (value == '\n') {
        if (inputTooLong_) {
            writeResponse(String("ERR ") + toString(ProtocolError::CommandTooLong) + " max=" + kMaxCommandLength);
            resetInput();
            return;
        }

        processLine(input_);
        resetInput();
        return;
    }

    if (inputTooLong_) {
        return;
    }

    if (input_.length() >= kMaxCommandLength) {
        inputTooLong_ = true;
        return;
    }

    input_ += value;
}

void SerialProtocol::writeResponse(const String& response)
{
    Serial.println(response);
}

void SerialProtocol::resetInput()
{
    input_ = "";
    inputTooLong_ = false;
}

}  // namespace stackchan::protocol
