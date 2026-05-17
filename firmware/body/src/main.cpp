#include <Arduino.h>

#include "body/BodyController.hpp"
#include "protocol/SerialProtocol.hpp"

stackchan::BodyController body;
stackchan::protocol::SerialProtocol protocol(body);

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("[BOOT] StackChan Body Firmware Phase 7");

    body.begin();
    protocol.begin();
}

void loop()
{
    body.update();
    protocol.update();
    delay(50);
}
