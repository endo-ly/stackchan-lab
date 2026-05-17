#pragma once

#include <Arduino.h>

namespace stackchan {

class InputState {
public:
    void updateLatest(uint32_t id, const String& type, const String& target, const String& value, uint32_t timestamp);
    void setTouchActive(bool active);
    void setButtonActive(bool active);
    void setImuMoving(bool moving);
    void clearEvents();

    uint32_t lastEventId() const;
    size_t eventCount() const;
    const String& latestType() const;
    const String& latestTarget() const;
    const String& latestValue() const;
    uint32_t latestTimestamp() const;
    bool touchActive() const;
    bool buttonActive() const;
    bool imuMoving() const;

private:
    uint32_t lastEventId_ = 0;
    size_t eventCount_ = 0;
    String latestType_ = "none";
    String latestTarget_ = "none";
    String latestValue_ = "none";
    uint32_t latestTimestamp_ = 0;
    bool touchActive_ = false;
    bool buttonActive_ = false;
    bool imuMoving_ = false;
};

}  // namespace stackchan
