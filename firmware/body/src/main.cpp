#include <Arduino.h>

#include "body/BodyController.hpp"
#include "network/NetworkController.hpp"
#include "protocol/SerialProtocol.hpp"

stackchan::BodyController body;
stackchan::network::NetworkController network(body);
stackchan::protocol::SerialProtocol protocol(body, &network);

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("[BOOT] StackChan Body Firmware Phase 8");

    body.begin();
    network.begin();
    protocol.begin();
}

void loop()
{
    body.update();
    network.update();
    protocol.update();
    delay(50);
}
