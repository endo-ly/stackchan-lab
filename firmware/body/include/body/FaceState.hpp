#pragma once

#include <Arduino.h>

#include "body/BodyTypes.hpp"

namespace stackchan {

class FaceState {
public:
    Expression expression() const;
    int gazeX() const;
    int gazeY() const;
    bool mouthOpen() const;
    bool isBlinking() const;
    bool isSpeaking() const;
    uint32_t lastBlinkAt() const;
    uint32_t nextBlinkDelayMs() const;
    uint32_t updatedAt() const;

    void setExpression(Expression expression);
    void setGaze(int x, int y);
    void setMouthOpen(bool open);
    void setBlinking(bool blinking);
    void setSpeaking(bool speaking);
    void setBlinkTiming(uint32_t lastBlinkAt, uint32_t nextBlinkDelayMs);
    void touch();

private:
    Expression expression_ = Expression::Neutral;
    int gazeX_ = 0;
    int gazeY_ = 0;
    bool mouthOpen_ = false;
    bool isBlinking_ = false;
    bool isSpeaking_ = false;
    uint32_t lastBlinkAt_ = 0;
    uint32_t nextBlinkDelayMs_ = 0;
    uint32_t updatedAt_ = 0;
};

}  // namespace stackchan
