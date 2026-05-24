#include "body/WakeState.hpp"

namespace stackchan {

bool WakeState::available() const { return available_; }
bool WakeState::modelLoaded() const { return modelLoaded_; }
bool WakeState::running() const { return running_; }
bool WakeState::detected() const { return detected_; }
const String& WakeState::engine() const { return engine_; }
const String& WakeState::modelName() const { return modelName_; }
const String& WakeState::wakeWord() const { return wakeWord_; }
uint32_t WakeState::modelBytes() const { return modelBytes_; }
uint8_t WakeState::lastProbability() const { return lastProbability_; }
uint8_t WakeState::averageProbability() const { return averageProbability_; }
uint32_t WakeState::queuedBlocks() const { return queuedBlocks_; }
uint32_t WakeState::processedBlocks() const { return processedBlocks_; }
uint32_t WakeState::featureFrames() const { return featureFrames_; }
uint32_t WakeState::inferenceRuns() const { return inferenceRuns_; }
uint32_t WakeState::lastSamplesConsumed() const { return lastSamplesConsumed_; }
uint32_t WakeState::lastMicRms() const { return lastMicRms_; }
int16_t WakeState::lastMicPeak() const { return lastMicPeak_; }
int8_t WakeState::lastFeatureMin() const { return lastFeatureMin_; }
int8_t WakeState::lastFeatureMax() const { return lastFeatureMax_; }
uint8_t WakeState::lastRawOutput() const { return lastRawOutput_; }
uint8_t WakeState::maxRawOutput() const { return maxRawOutput_; }
uint8_t WakeState::micRecordingState() const { return micRecordingState_; }
const String& WakeState::lastError() const { return lastError_; }
uint32_t WakeState::startedAtMs() const { return startedAtMs_; }
uint32_t WakeState::lastDetectedAtMs() const { return lastDetectedAtMs_; }

void WakeState::setEngine(const String& engine) { engine_ = engine; }
void WakeState::setModelName(const String& modelName) { modelName_ = modelName; }
void WakeState::setWakeWord(const String& wakeWord) { wakeWord_ = wakeWord; }
void WakeState::setModelLoaded(bool modelLoaded) { modelLoaded_ = modelLoaded; }
void WakeState::setModelBytes(uint32_t modelBytes) { modelBytes_ = modelBytes; }
void WakeState::setProbabilities(uint8_t lastProbability, uint8_t averageProbability)
{
    lastProbability_ = lastProbability;
    averageProbability_ = averageProbability;
}
void WakeState::setDiagnostics(uint32_t queuedBlocks, uint32_t processedBlocks, uint32_t featureFrames, uint32_t inferenceRuns, uint32_t lastSamplesConsumed, uint8_t micRecordingState)
{
    queuedBlocks_ = queuedBlocks;
    processedBlocks_ = processedBlocks;
    featureFrames_ = featureFrames;
    inferenceRuns_ = inferenceRuns;
    lastSamplesConsumed_ = lastSamplesConsumed;
    micRecordingState_ = micRecordingState;
}
void WakeState::setSignalDiagnostics(uint32_t lastMicRms, int16_t lastMicPeak, int8_t lastFeatureMin, int8_t lastFeatureMax, uint8_t lastRawOutput, uint8_t maxRawOutput)
{
    lastMicRms_ = lastMicRms;
    lastMicPeak_ = lastMicPeak;
    lastFeatureMin_ = lastFeatureMin;
    lastFeatureMax_ = lastFeatureMax;
    lastRawOutput_ = lastRawOutput;
    maxRawOutput_ = maxRawOutput;
}
void WakeState::setAvailable(bool available) { available_ = available; }
void WakeState::setRunning(bool running) { running_ = running; }
void WakeState::setDetected(bool detected) { detected_ = detected; }
void WakeState::setStartedAtMs(uint32_t startedAtMs) { startedAtMs_ = startedAtMs; }
void WakeState::setLastDetectedAtMs(uint32_t lastDetectedAtMs) { lastDetectedAtMs_ = lastDetectedAtMs; }
void WakeState::setError(const String& error) { lastError_ = error; }
void WakeState::clearError() { lastError_ = ""; }

}  // namespace stackchan
