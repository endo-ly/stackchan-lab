#include "body/DisplayController.hpp"

#include <M5StackChan.h>

namespace stackchan {

void DisplayController::begin()
{
    Serial.println("[DISPLAY] begin");
    M5StackChan.Display().setRotation(1);
}

void DisplayController::showBoot()
{
    Serial.println("[DISPLAY] show boot");
    clear();
    drawCenteredLine("StackChan", 72, 3, TFT_WHITE);
    drawCenteredLine("Body Firmware", 116, 2, TFT_CYAN);
    drawCenteredLine("Phase 2", 154, 2, TFT_GREENYELLOW);
}

void DisplayController::showReady()
{
    Serial.println("[DISPLAY] show ready");
    clear();
    drawCenteredLine("Ready", 74, 3, TFT_GREENYELLOW);
    drawCenteredLine("Mode: Idle", 122, 2, TFT_WHITE);
    drawCenteredLine("Face: Neutral", 152, 2, TFT_CYAN);
    drawCenteredLine("Mood: Calm", 182, 2, TFT_CYAN);
}

void DisplayController::showStatus(const BodyState& state)
{
    Serial.println("[DISPLAY] show status");
    clear();
    drawCenteredLine("Status", 52, 2, TFT_WHITE);
    drawCenteredLine(toString(state.mode()), 88, 2, TFT_GREENYELLOW);
    drawCenteredLine(toString(state.expression()), 124, 2, TFT_CYAN);
    drawCenteredLine(toString(state.mood()), 160, 2, TFT_CYAN);
    drawCenteredLine(toString(state.pose()), 196, 2, TFT_WHITE);
}

void DisplayController::showExpression(Expression expression)
{
    Serial.print("[DISPLAY] show expression=");
    Serial.println(toString(expression));

    clear();
    drawCenteredLine("Expression:", 88, 2, TFT_WHITE);
    drawCenteredLine(toString(expression), 138, 3, TFT_CYAN);
}

void DisplayController::showError(const char* message)
{
    Serial.print("[DISPLAY] show error=");
    Serial.println(message);

    clear();
    drawCenteredLine("Error", 82, 3, TFT_RED);
    drawCenteredLine(message, 140, 1, TFT_WHITE);
}

void DisplayController::update()
{
}

void DisplayController::clear()
{
    auto& display = M5StackChan.Display();
    display.fillScreen(TFT_BLACK);
    display.setTextDatum(middle_center);
}

void DisplayController::drawCenteredLine(const char* text, int y, int textSize, uint32_t color)
{
    auto& display = M5StackChan.Display();
    display.setTextColor(color, TFT_BLACK);
    display.setTextSize(textSize);
    display.drawString(text, display.width() / 2, y);
}

}  // namespace stackchan

