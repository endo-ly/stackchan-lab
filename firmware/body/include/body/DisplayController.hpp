#pragma once

#include "body/BodyState.hpp"
#include "body/BodyTypes.hpp"

namespace stackchan {

class DisplayController {
public:
    void begin();
    void showBoot();
    void showReady();
    void showStatus(const BodyState& state);
    void showExpression(Expression expression);
    void showError(const char* message);
    void update();

private:
    void clear();
    void drawCenteredLine(const char* text, int y, int textSize, uint32_t color);
};

}  // namespace stackchan

