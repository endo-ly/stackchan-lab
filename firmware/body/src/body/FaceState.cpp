#include "body/FaceState.hpp"

namespace stackchan {

Expression FaceState::expression() const { return expression_; }
int FaceState::gazeX() const { return gazeX_; }
int FaceState::gazeY() const { return gazeY_; }
bool FaceState::mouthOpen() const { return mouthOpen_; }
bool FaceState::isBlinking() const { return isBlinking_; }
bool FaceState::isSpeaking() const { return isSpeaking_; }
uint32_t FaceState::lastBlinkAt() const { return lastBlinkAt_; }
uint32_t FaceState::nextBlinkDelayMs() const { return nextBlinkDelayMs_; }
uint32_t FaceState::updatedAt() const { return updatedAt_; }

void FaceState::setExpression(Expression expression)
{
    expression_ = expression;
    touch();
}

void FaceState::setGaze(int x, int y)
{
    gazeX_ = x;
    gazeY_ = y;
    touch();
}

void FaceState::setMouthOpen(bool open)
{
    mouthOpen_ = open;
    touch();
}

void FaceState::setBlinking(bool blinking)
{
    isBlinking_ = blinking;
    touch();
}

void FaceState::setSpeaking(bool speaking)
{
    isSpeaking_ = speaking;
    touch();
}

void FaceState::setBlinkTiming(uint32_t lastBlinkAt, uint32_t nextBlinkDelayMs)
{
    lastBlinkAt_ = lastBlinkAt;
    nextBlinkDelayMs_ = nextBlinkDelayMs;
    touch();
}

void FaceState::touch()
{
    updatedAt_ = millis();
}

}  // namespace stackchan
