#include "body/InputController.hpp"

#include <M5StackChan.h>

namespace stackchan {
namespace {

constexpr uint32_t kTouchDebounceMs = 200;
constexpr uint32_t kButtonDebounceMs = 200;
constexpr uint32_t kImuRateLimitMs = 1000;
constexpr float kMovedAccelDelta = 0.20F;
constexpr float kShakeAccelDelta = 0.65F;
constexpr float kMovedGyroDps = 35.0F;
constexpr float kShakeGyroDps = 140.0F;

bool enoughTime(uint32_t now, uint32_t last, uint32_t interval)
{
    return last == 0 || now - last >= interval;
}

}  // namespace

void InputController::begin()
{
    Serial.println("[INPUT] begin");
}

void InputController::update()
{
    updateTouch();
    updateButtons();
    updateImu();
}

size_t InputController::eventCount() const
{
    return count_;
}

bool InputController::hasEvents() const
{
    return count_ > 0;
}

bool InputController::getEvent(size_t index, InputEvent& event) const
{
    if (index >= count_) {
        return false;
    }
    event = events_[(head_ + index) % kMaxInputEvents];
    return true;
}

bool InputController::getLatestEvent(InputEvent& event) const
{
    if (count_ == 0) {
        return false;
    }
    event = events_[(head_ + count_ - 1) % kMaxInputEvents];
    return true;
}

void InputController::clearEvents()
{
    head_ = 0;
    count_ = 0;
    state_.clearEvents();
}

const InputState& InputController::getState() const
{
    return state_;
}

void InputController::updateTouch()
{
    if (!M5.Touch.isEnabled()) {
        state_.setTouchActive(false);
        return;
    }

    const auto& detail = M5.Touch.getDetail();
    const uint32_t now = millis();
    state_.setTouchActive(detail.isPressed());

    if (detail.wasClicked() && enoughTime(now, lastTouchEventAt_, kTouchDebounceMs)) {
        lastTouchEventAt_ = now;
        pushTouchEvent("tap", now, detail.x, detail.y);
    }
}

void InputController::updateButtons()
{
    const uint32_t now = millis();
    state_.setButtonActive(M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnC.isPressed());

    struct ButtonTarget {
        m5::Button_Class* button;
        const char* target;
    };
    ButtonTarget buttons[] = {
        {&M5.BtnA, "button_a"},
        {&M5.BtnB, "button_b"},
        {&M5.BtnC, "button_c"},
    };

    for (const auto& entry : buttons) {
        if (entry.button->wasClicked() && enoughTime(now, lastButtonEventAt_, kButtonDebounceMs)) {
            lastButtonEventAt_ = now;
            pushEvent("button", entry.target, "tap", now);
            return;
        }
    }
}

void InputController::updateImu()
{
    if (!M5.Imu.isEnabled()) {
        state_.setImuMoving(false);
        return;
    }

    float ax = 0.0F;
    float ay = 0.0F;
    float az = 0.0F;
    if (!M5.Imu.getAccel(&ax, &ay, &az)) {
        state_.setImuMoving(false);
        return;
    }

    float gx = 0.0F;
    float gy = 0.0F;
    float gz = 0.0F;
    const bool hasGyro = M5.Imu.getGyro(&gx, &gy, &gz);

    const float accelMagnitude = sqrtf(ax * ax + ay * ay + az * az);
    const float accelDelta = fabsf(accelMagnitude - 1.0F);
    const float gyroMagnitude = hasGyro ? sqrtf(gx * gx + gy * gy + gz * gz) : 0.0F;
    const bool moving = accelDelta >= kMovedAccelDelta || gyroMagnitude >= kMovedGyroDps;
    state_.setImuMoving(moving);

    const uint32_t now = millis();
    if (!moving) {
        return;
    }
    if (!enoughTime(now, lastImuEventAt_, kImuRateLimitMs)) {
        return;
    }

    lastImuEventAt_ = now;
    const bool shaking = accelDelta >= kShakeAccelDelta || gyroMagnitude >= kShakeGyroDps;
    pushEvent("imu", "device", shaking ? "shake" : "moved", now);
}

void InputController::pushEvent(const String& type, const String& target, const String& value, uint32_t timestamp)
{
    const size_t writeIndex = (head_ + count_) % kMaxInputEvents;
    events_[writeIndex] = InputEvent{nextId_++, type, target, value, timestamp};
    if (count_ < kMaxInputEvents) {
        count_++;
    } else {
        head_ = (head_ + 1) % kMaxInputEvents;
    }
    state_.updateLatest(events_[writeIndex].id, type, target, value, timestamp);
}

void InputController::pushTouchEvent(const String& value, uint32_t timestamp, int x, int y)
{
    const size_t writeIndex = (head_ + count_) % kMaxInputEvents;
    events_[writeIndex] = InputEvent{nextId_++, "touch", "screen", value, timestamp, x, y};
    if (count_ < kMaxInputEvents) {
        count_++;
    } else {
        head_ = (head_ + 1) % kMaxInputEvents;
    }
    state_.updateLatest(events_[writeIndex].id, "touch", "screen", value, timestamp);
}

}  // namespace stackchan
