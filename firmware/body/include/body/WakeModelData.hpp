#pragma once

#include <Arduino.h>

namespace stackchan {

extern const unsigned char kWakeModelData[];
extern const unsigned int kWakeModelDataLen;

constexpr const char* kWakeModelName = "okay_nabu";
constexpr const char* kWakeWordName = "Okay Nabu";
constexpr float kWakeProbabilityCutoff = 0.87F;
constexpr uint8_t kWakeSlidingWindowSize = 5;
constexpr uint32_t kWakeFeatureStepMs = 10;
constexpr size_t kWakeTensorArenaSize = 26080;
constexpr size_t kWakeVariableArenaSize = 1024;
constexpr uint8_t kWakeFeatureSize = 40;
constexpr uint8_t kWakeFeatureDurationMs = 30;
constexpr uint8_t kWakeMinSlicesBeforeDetection = 10;

}  // namespace stackchan
