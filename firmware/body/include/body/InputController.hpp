#pragma once

#include <Arduino.h>

#include "body/InputEvent.hpp"
#include "body/InputState.hpp"

namespace stackchan {

constexpr size_t kMaxInputEvents = 16;

class InputController {
public:
    void begin();
    void update();

    size_t eventCount() const;
    bool hasEvents() const;
    bool getEvent(size_t index, InputEvent& event) const;
    bool getLatestEvent(InputEvent& event) const;
    void clearEvents();
    const InputState& getState() const;

private:
    void updateTouch();
    void updateButtons();
    void updateImu();
    void pushEvent(const String& type, const String& target, const String& value, uint32_t timestamp);
    void pushTouchEvent(const String& value, uint32_t timestamp, int x, int y);

    InputState state_;
    InputEvent events_[kMaxInputEvents];
    size_t head_ = 0;
    size_t count_ = 0;
    uint32_t nextId_ = 1;
    uint32_t lastTouchEventAt_ = 0;
    uint32_t lastButtonEventAt_ = 0;
    uint32_t lastImuEventAt_ = 0;
};

}  // namespace stackchan
