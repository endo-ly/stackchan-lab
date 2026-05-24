#pragma once

#include <Arduino.h>

namespace stackchan {

class WakeState {
public:
    bool available() const;
    bool modelLoaded() const;
    bool running() const;
    bool detected() const;
    const String& engine() const;
    const String& modelName() const;
    const String& wakeWord() const;
    uint32_t modelBytes() const;
    uint8_t lastProbability() const;
    uint8_t averageProbability() const;
    uint32_t queuedBlocks() const;
    uint32_t processedBlocks() const;
    uint32_t featureFrames() const;
    uint32_t inferenceRuns() const;
    uint32_t lastSamplesConsumed() const;
    uint32_t lastMicRms() const;
    int16_t lastMicPeak() const;
    int8_t lastFeatureMin() const;
    int8_t lastFeatureMax() const;
    uint8_t lastRawOutput() const;
    uint8_t maxRawOutput() const;
    uint8_t micRecordingState() const;
    const String& lastError() const;
    uint32_t startedAtMs() const;
    uint32_t lastDetectedAtMs() const;

    void setEngine(const String& engine);
    void setModelName(const String& modelName);
    void setWakeWord(const String& wakeWord);
    void setModelLoaded(bool modelLoaded);
    void setModelBytes(uint32_t modelBytes);
    void setProbabilities(uint8_t lastProbability, uint8_t averageProbability);
    void setDiagnostics(uint32_t queuedBlocks, uint32_t processedBlocks, uint32_t featureFrames, uint32_t inferenceRuns, uint32_t lastSamplesConsumed, uint8_t micRecordingState);
    void setSignalDiagnostics(uint32_t lastMicRms, int16_t lastMicPeak, int8_t lastFeatureMin, int8_t lastFeatureMax, uint8_t lastRawOutput, uint8_t maxRawOutput);
    void setAvailable(bool available);
    void setRunning(bool running);
    void setDetected(bool detected);
    void setStartedAtMs(uint32_t startedAtMs);
    void setLastDetectedAtMs(uint32_t lastDetectedAtMs);
    void setError(const String& error);
    void clearError();

private:
    bool available_ = false;
    bool modelLoaded_ = false;
    bool running_ = false;
    bool detected_ = false;
    String engine_ = "microWakeWord";
    String modelName_;
    String wakeWord_ = "stackchan";
    uint32_t modelBytes_ = 0;
    uint8_t lastProbability_ = 0;
    uint8_t averageProbability_ = 0;
    uint32_t queuedBlocks_ = 0;
    uint32_t processedBlocks_ = 0;
    uint32_t featureFrames_ = 0;
    uint32_t inferenceRuns_ = 0;
    uint32_t lastSamplesConsumed_ = 0;
    uint32_t lastMicRms_ = 0;
    int16_t lastMicPeak_ = 0;
    int8_t lastFeatureMin_ = 0;
    int8_t lastFeatureMax_ = 0;
    uint8_t lastRawOutput_ = 0;
    uint8_t maxRawOutput_ = 0;
    uint8_t micRecordingState_ = 0;
    String lastError_;
    uint32_t startedAtMs_ = 0;
    uint32_t lastDetectedAtMs_ = 0;
};

}  // namespace stackchan
