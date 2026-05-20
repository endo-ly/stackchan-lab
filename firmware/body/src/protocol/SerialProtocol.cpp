#include "protocol/SerialProtocol.hpp"

namespace stackchan::protocol {

SerialProtocol::SerialProtocol(BodyController& body, network::NetworkController* network)
    : handler_(body, network)
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
    const String response = handler_.handle(command);
    writeResponse(response);
    maybeReceiveWav(response);
    maybeReceiveWifiJson(response);
    maybeReceiveDeviceConfigJson(response);
}

bool SerialProtocol::maybeReceiveDeviceConfigJson(const String& response)
{
    size_t size = 0;
    if (!parseReadyDeviceConfigJsonSize(response, size)) {
        return false;
    }
    String json;
    json.reserve(size);
    const uint32_t startedAt = millis();
    while (json.length() < size) {
        if (Serial.available() > 0) {
            json += static_cast<char>(Serial.read());
            continue;
        }
        if (millis() - startedAt > 5000) {
            writeResponse("ERR DEVICE_CONFIG_JSON_RECEIVE_TIMEOUT");
            return false;
        }
        delay(1);
    }
    writeResponse(handler_.completeDeviceConfigJsonTransfer(json));
    return true;
}

bool SerialProtocol::maybeReceiveWav(const String& response)
{
    size_t size = 0;
    if (!parseReadyWavSize(response, size)) {
        return false;
    }

    uint8_t* buffer = handler_.wavReceiveBuffer();
    if (buffer == nullptr) {
        writeResponse("ERR AUDIO_TRANSFER_FAILED");
        return false;
    }

    const uint32_t startedAt = millis();
    const uint32_t timeoutMs = 5000 + static_cast<uint32_t>(size / 10);
    size_t received = 0;
    while (received < size) {
        if (Serial.available() > 0) {
            received += Serial.readBytes(buffer + received, size - received);
            continue;
        }
        if (millis() - startedAt > timeoutMs) {
            writeResponse("ERR AUDIO_RECEIVE_TIMEOUT");
            return false;
        }
        delay(1);
    }

    writeResponse(handler_.completeWavTransfer(size));
    return true;
}

bool SerialProtocol::maybeReceiveWifiJson(const String& response)
{
    size_t size = 0;
    if (!parseReadyWifiJsonSize(response, size)) {
        return false;
    }
    String json;
    json.reserve(size);
    const uint32_t startedAt = millis();
    while (json.length() < size) {
        if (Serial.available() > 0) {
            json += static_cast<char>(Serial.read());
            continue;
        }
        if (millis() - startedAt > 5000) {
            writeResponse("ERR WIFI_JSON_RECEIVE_TIMEOUT");
            return false;
        }
        delay(1);
    }
    writeResponse(handler_.completeWifiJsonTransfer(json));
    return true;
}

bool SerialProtocol::parseReadyWavSize(const String& response, size_t& size) const
{
    if (!response.startsWith("READY AUDIO WAV size=")) {
        return false;
    }

    const String value = response.substring(String("READY AUDIO WAV size=").length());
    const int parsed = value.toInt();
    if (parsed <= 0) {
        return false;
    }
    size = static_cast<size_t>(parsed);
    return true;
}

bool SerialProtocol::parseReadyWifiJsonSize(const String& response, size_t& size) const
{
    if (!response.startsWith("READY WIFI JSON size=")) {
        return false;
    }
    const String value = response.substring(String("READY WIFI JSON size=").length());
    const int parsed = value.toInt();
    if (parsed <= 0) {
        return false;
    }
    size = static_cast<size_t>(parsed);
    return true;
}

bool SerialProtocol::parseReadyDeviceConfigJsonSize(const String& response, size_t& size) const
{
    if (!response.startsWith("READY DEVICE CONFIG JSON size=")) {
        return false;
    }
    const String value = response.substring(String("READY DEVICE CONFIG JSON size=").length());
    const int parsed = value.toInt();
    if (parsed <= 0) {
        return false;
    }
    size = static_cast<size_t>(parsed);
    return true;
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
