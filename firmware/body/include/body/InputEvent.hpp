#pragma once

#include <Arduino.h>

namespace stackchan {

struct InputEvent {
    uint32_t id = 0;
    String type;
    String target;
    String value;
    uint32_t timestamp = 0;
    int x = -1;
    int y = -1;
};

String formatInputEvent(const InputEvent& event);

}  // namespace stackchan
