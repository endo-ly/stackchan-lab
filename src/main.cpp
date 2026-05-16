#include <Arduino.h>
#include <M5StackChan.h>

namespace {

constexpr uint8_t kReadyLedRed = 0;
constexpr uint8_t kReadyLedGreen = 12;
constexpr uint8_t kReadyLedBlue = 18;
constexpr int kHomeMoveSpeed = 200;

void drawBootScreen(const char* status)
{
    auto& display = M5StackChan.Display();

    display.fillScreen(TFT_BLACK);
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setTextDatum(middle_center);
    display.setTextSize(3);
    display.drawString("StackChan Lab", display.width() / 2, display.height() / 2 - 34);

    display.setTextSize(2);
    display.setTextColor(TFT_CYAN, TFT_BLACK);
    display.drawString("Phase 1", display.width() / 2, display.height() / 2 + 4);

    display.setTextSize(1);
    display.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    display.drawString(status, display.width() / 2, display.height() / 2 + 42);
}

void initializeDisplay()
{
    Serial.println("Initializing display...");
    M5StackChan.Display().setRotation(1);
    drawBootScreen("Initializing...");
}

void initializeRgbLed()
{
    Serial.println("Initializing RGB LED...");
    M5StackChan.showRgbColor(kReadyLedRed, kReadyLedGreen, kReadyLedBlue);
}

void initializeMotion()
{
    Serial.println("Initializing motion...");
    M5StackChan.setServoPowerEnabled(true);
    M5StackChan.Motion.goHome(kHomeMoveSpeed);
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("Booting StackChan Lab...");

    M5StackChan.begin();

    initializeDisplay();
    initializeRgbLed();
    initializeMotion();

    drawBootScreen("Ready");
    Serial.println("Ready.");
}

void loop()
{
    M5StackChan.update();
    delay(50);
}
