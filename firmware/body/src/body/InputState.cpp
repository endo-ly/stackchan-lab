#include "body/InputState.hpp"

namespace stackchan {

void InputState::updateLatest(uint32_t id, const String& type, const String& target, const String& value, uint32_t timestamp)
{
    lastEventId_ = id;
    eventCount_++;
    latestType_ = type;
    latestTarget_ = target;
    latestValue_ = value;
    latestTimestamp_ = timestamp;
}

void InputState::setTouchActive(bool active) { touchActive_ = active; }
void InputState::setButtonActive(bool active) { buttonActive_ = active; }
void InputState::setImuMoving(bool moving) { imuMoving_ = moving; }

void InputState::clearEvents()
{
    eventCount_ = 0;
    latestType_ = "none";
    latestTarget_ = "none";
    latestValue_ = "none";
    latestTimestamp_ = 0;
}

uint32_t InputState::lastEventId() const { return lastEventId_; }
size_t InputState::eventCount() const { return eventCount_; }
const String& InputState::latestType() const { return latestType_; }
const String& InputState::latestTarget() const { return latestTarget_; }
const String& InputState::latestValue() const { return latestValue_; }
uint32_t InputState::latestTimestamp() const { return latestTimestamp_; }
bool InputState::touchActive() const { return touchActive_; }
bool InputState::buttonActive() const { return buttonActive_; }
bool InputState::imuMoving() const { return imuMoving_; }

}  // namespace stackchan
