#include "body/InputEvent.hpp"

namespace stackchan {

String formatInputEvent(const InputEvent& event)
{
    String line = "EVENT id=";
    line += event.id;
    line += " type=";
    line += event.type;
    line += " target=";
    line += event.target;
    line += " value=";
    line += event.value;
    line += " timestamp=";
    line += event.timestamp;
    if (event.x >= 0 && event.y >= 0) {
        line += " x=";
        line += event.x;
        line += " y=";
        line += event.y;
    }
    return line;
}

}  // namespace stackchan
