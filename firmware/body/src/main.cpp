#include <Arduino.h>

#include "body/BodyController.hpp"

stackchan::BodyController body;

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("[BOOT] StackChan Body Firmware Phase 2");

    body.begin();
}

void loop()
{
    body.update();
    delay(50);
}
