#include "body/LedController.hpp"

#include <M5StackChan.h>

namespace stackchan {

void LedController::begin()
{
    Serial.println("[LED] begin");
    off();
}

void LedController::setMood(Mood mood)
{
    Serial.print("[LED] mood=");
    Serial.println(toString(mood));

    switch (mood) {
        case Mood::Calm:
            setColor(0, 10, 16);
            break;
        case Mood::Active:
            setColor(8, 18, 0);
            break;
        case Mood::Speaking:
            setColor(12, 8, 16);
            break;
        case Mood::Warning:
            setColor(18, 6, 0);
            break;
        case Mood::Off:
            off();
            break;
    }
}

void LedController::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    M5StackChan.showRgbColor(r, g, b);
}

void LedController::off()
{
    setColor(0, 0, 0);
}

void LedController::update()
{
}

}  // namespace stackchan

