#pragma once

#include <Arduino.h>

#include "body/BodyTypes.hpp"

namespace stackchan {

class LedController {
public:
    void begin();
    void setMood(Mood mood);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void off();
    void update();
};

}  // namespace stackchan

