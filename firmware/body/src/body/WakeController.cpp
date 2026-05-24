#include "body/WakeController.hpp"

#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

#include <M5StackChan.h>
#include <tensorflow/lite/kernels/internal/tensor_ctypes.h>

#include "body/WakeModelData.hpp"

namespace stackchan {
namespace {

uint8_t probabilityCutoff()
{
    return static_cast<uint8_t>((kWakeProbabilityCutoff * 255.0F) + 0.5F);
}

int8_t scaleFeature(uint16_t feature)
{
    constexpr int32_t valueScale = 256;
    constexpr int32_t valueDiv = 666;
    int32_t value = ((static_cast<int32_t>(feature) * valueScale) + (valueDiv / 2)) / valueDiv;
    value += INT8_MIN;
    if (value < INT8_MIN) return INT8_MIN;
    if (value > INT8_MAX) return INT8_MAX;
    return static_cast<int8_t>(value);
}

void measureMicBlock(const int16_t* samples, size_t sampleCount, uint32_t& rms, int16_t& peak)
{
    uint64_t sumSquares = 0;
    int32_t maxAbs = 0;
    for (size_t i = 0; i < sampleCount; ++i) {
        const int32_t value = samples[i];
        const int32_t absValue = value < 0 ? -value : value;
        if (absValue > maxAbs) {
            maxAbs = absValue;
        }
        sumSquares += static_cast<uint64_t>(value * value);
    }
    rms = sampleCount == 0 ? 0 : static_cast<uint32_t>(sqrt(static_cast<double>(sumSquares / sampleCount)));
    peak = static_cast<int16_t>(maxAbs > INT16_MAX ? INT16_MAX : maxAbs);
}

}  // namespace

void WakeController::begin()
{
    state_.setEngine("microWakeWord");
    state_.setModelName(kWakeModelName);
    state_.setWakeWord(kWakeWordName);
    state_.setModelBytes(kWakeModelDataLen);
    state_.setAvailable(false);
    state_.setModelLoaded(false);
    state_.setRunning(false);
    state_.setDetected(false);
    state_.setProbabilities(0, 0);
    state_.setError("WAKE_MODEL_NOT_LOADED");

    FrontendFillConfigWithDefaults(&frontendConfig_);
    frontendConfig_.window.size_ms = kWakeFeatureDurationMs;
    frontendConfig_.window.step_size_ms = kWakeFeatureStepMs;
    frontendConfig_.filterbank.num_channels = kWakeFeatureSize;
    frontendConfig_.filterbank.lower_band_limit = 125.0F;
    frontendConfig_.filterbank.upper_band_limit = 7500.0F;
    frontendConfig_.noise_reduction.smoothing_bits = 10;
    frontendConfig_.noise_reduction.even_smoothing = 0.025F;
    frontendConfig_.noise_reduction.odd_smoothing = 0.06F;
    frontendConfig_.noise_reduction.min_signal_remaining = 0.05F;
    frontendConfig_.pcan_gain_control.enable_pcan = true;
    frontendConfig_.pcan_gain_control.strength = 0.95F;
    frontendConfig_.pcan_gain_control.offset = 80.0F;
    frontendConfig_.pcan_gain_control.gain_bits = 21;
    frontendConfig_.log_scale.enable_log = true;
    frontendConfig_.log_scale.scale_shift = 6;
    registerModelOps();

    String error;
    if (!loadModel(error)) {
        state_.setError(error);
        return;
    }

    state_.setModelLoaded(true);
    state_.setAvailable(true);
    state_.clearError();
}

void WakeController::update()
{
    if (!state_.running()) {
        return;
    }

    state_.setDiagnostics(queuedBlocks_, processedBlocks_, featureFrames_, inferenceRuns_, lastSamplesConsumed_, M5.Mic.isRecording());
    if (micChunkPending_) {
        processMicChunk();
    }
    queueMicChunk();
}

bool WakeController::start(String& error)
{
    if (!state_.available()) {
        error = state_.lastError().length() > 0 ? state_.lastError() : "WAKE_NOT_AVAILABLE";
        return false;
    }
    if (!startFrontend(error)) {
        state_.setError(error);
        return false;
    }

    state_.setRunning(true);
    state_.setDetected(false);
    state_.setProbabilities(0, 0);
    queuedBlocks_ = 0;
    processedBlocks_ = 0;
    featureFrames_ = 0;
    inferenceRuns_ = 0;
    lastSamplesConsumed_ = 0;
    maxRawOutput_ = 0;
    state_.setDiagnostics(0, 0, 0, 0, 0, M5.Mic.isRecording());
    state_.setSignalDiagnostics(0, 0, 0, 0, 0, 0, 0, 0, 0, INT8_MIN, 0, 0);
    state_.setStartedAtMs(millis());
    state_.clearError();
    resetProbabilities();
    if (interpreter_->Reset() != kTfLiteOk) {
        error = "WAKE_INTERPRETER_RESET_FAILED";
        state_.setError(error);
        state_.setRunning(false);
        stopFrontend();
        return false;
    }
    return true;
}

void WakeController::stop()
{
    stopFrontend();
    micChunkPending_ = false;
    state_.setRunning(false);
}

const WakeState& WakeController::getState() const
{
    return state_;
}

bool WakeController::loadModel(String& error)
{
    model_ = tflite::GetModel(kWakeModelData);
    if (model_ == nullptr) {
        error = "WAKE_MODEL_INVALID";
        return false;
    }
    if (model_->version() != TFLITE_SCHEMA_VERSION) {
        error = "WAKE_MODEL_SCHEMA_MISMATCH";
        return false;
    }

    tensorArena_ = allocateArena(kWakeTensorArenaSize);
    if (tensorArena_ == nullptr) {
        error = "WAKE_TENSOR_ARENA_ALLOC_FAILED";
        return false;
    }

    variableArena_ = allocateArena(kWakeVariableArenaSize);
    if (variableArena_ == nullptr) {
        error = "WAKE_VARIABLE_ARENA_ALLOC_FAILED";
        return false;
    }
    variableAllocator_ = tflite::MicroAllocator::Create(variableArena_, kWakeVariableArenaSize);
    resourceVariables_ = tflite::MicroResourceVariables::Create(variableAllocator_, 20);
    if (variableAllocator_ == nullptr || resourceVariables_ == nullptr) {
        error = "WAKE_RESOURCE_VARIABLE_ALLOC_FAILED";
        return false;
    }

    interpreter_ = new (std::nothrow) tflite::MicroInterpreter(model_, opResolver_, tensorArena_, kWakeTensorArenaSize, resourceVariables_);
    if (interpreter_ == nullptr) {
        error = "WAKE_INTERPRETER_ALLOC_FAILED";
        return false;
    }

    if (interpreter_->AllocateTensors() != kTfLiteOk) {
        error = "WAKE_ALLOCATE_TENSORS_FAILED";
        return false;
    }

    if (interpreter_->inputs_size() != 1 || interpreter_->outputs_size() != 1) {
        error = "WAKE_MODEL_IO_UNSUPPORTED";
        return false;
    }

    const TfLiteTensor* input = interpreter_->input(0);
    const TfLiteTensor* output = interpreter_->output(0);
    if (input == nullptr || output == nullptr) {
        error = "WAKE_MODEL_TENSOR_MISSING";
        return false;
    }
    if (input->type != kTfLiteInt8 || output->type != kTfLiteUInt8) {
        error = "WAKE_MODEL_TENSOR_TYPE_UNSUPPORTED";
        return false;
    }
    if (input->dims->size != 3 || input->dims->data[0] != 1 || input->dims->data[2] != kWakeFeatureSize) {
        error = "WAKE_MODEL_INPUT_SHAPE_UNSUPPORTED";
        return false;
    }
    if (output->dims->size != 2 || output->dims->data[0] != 1 || output->dims->data[1] != 1) {
        error = "WAKE_MODEL_OUTPUT_SHAPE_UNSUPPORTED";
        return false;
    }

    return true;
}

bool WakeController::startFrontend(String& error)
{
    if (frontendReady_) {
        return true;
    }
    if (!FrontendPopulateState(&frontendConfig_, &frontendState_, 16000)) {
        error = "WAKE_FRONTEND_ALLOC_FAILED";
        return false;
    }
    frontendReady_ = true;
    return true;
}

void WakeController::stopFrontend()
{
    if (frontendReady_) {
        FrontendFreeStateContents(&frontendState_);
        frontendReady_ = false;
    }
}

void WakeController::queueMicChunk()
{
    if (micChunkPending_ || M5.Mic.isRecording() >= 2) {
        return;
    }
    if (!M5.Mic.record(micChunk_, sizeof(micChunk_) / sizeof(micChunk_[0]), 16000, false)) {
        state_.setError("WAKE_MIC_RECORD_FAILED");
        state_.setRunning(false);
        stopFrontend();
        return;
    }
    micChunkPending_ = true;
    queuedBlocks_ += kMicBlocksPerChunk;
    state_.setDiagnostics(queuedBlocks_, processedBlocks_, featureFrames_, inferenceRuns_, lastSamplesConsumed_, M5.Mic.isRecording());
}

void WakeController::processMicChunk()
{
    if (M5.Mic.isRecording() != 0) {
        return;
    }
    micChunkPending_ = false;
    uint32_t rms = 0;
    int16_t peak = 0;
    measureMicBlock(micChunk_, sizeof(micChunk_) / sizeof(micChunk_[0]), rms, peak);
    const uint32_t maxRms = rms > state_.maxMicRms() ? rms : state_.maxMicRms();
    const int16_t maxPeak = peak > state_.maxMicPeak() ? peak : state_.maxMicPeak();
    state_.setSignalDiagnostics(rms, peak, maxRms, maxPeak, state_.lastFeatureRawMin(), state_.lastFeatureRawMax(), state_.maxFeatureRawMax(), state_.lastFeatureMin(), state_.lastFeatureMax(), state_.maxFeatureMax(), state_.lastRawOutput(), maxRawOutput_);

    String error;
    for (size_t i = 0; i < kMicBlocksPerChunk; ++i) {
        const int16_t* block = micChunk_ + (i * kMicBlockSamples);
        ++processedBlocks_;
        if (!processFeatures(block, kMicBlockSamples, error)) {
            state_.setError(error);
            state_.setRunning(false);
            stopFrontend();
            return;
        }
        if (!state_.running()) {
            return;
        }
    }
}

bool WakeController::processFeatures(const int16_t* samples, size_t sampleCount, String& error)
{
    size_t consumed = 0;
    struct FrontendOutput output = FrontendProcessSamples(&frontendState_, samples, sampleCount, &consumed);
    lastSamplesConsumed_ = consumed;
    state_.setDiagnostics(queuedBlocks_, processedBlocks_, featureFrames_, inferenceRuns_, lastSamplesConsumed_, M5.Mic.isRecording());
    if (output.size == 0) {
        return true;
    }
    if (output.size != kWakeFeatureSize) {
        error = "WAKE_FEATURE_SIZE_UNSUPPORTED";
        return false;
    }

    int8_t features[kWakeFeatureSize] = {};
    uint16_t rawMin = UINT16_MAX;
    uint16_t rawMax = 0;
    int8_t featureMin = INT8_MAX;
    int8_t featureMax = INT8_MIN;
    for (size_t i = 0; i < output.size; ++i) {
        if (output.values[i] < rawMin) {
            rawMin = output.values[i];
        }
        if (output.values[i] > rawMax) {
            rawMax = output.values[i];
        }
        features[i] = scaleFeature(output.values[i]);
        if (features[i] < featureMin) {
            featureMin = features[i];
        }
        if (features[i] > featureMax) {
            featureMax = features[i];
        }
    }
    ++featureFrames_;
    const uint16_t maxRawMax = rawMax > state_.maxFeatureRawMax() ? rawMax : state_.maxFeatureRawMax();
    const int8_t maxFeatureMax = featureMax > state_.maxFeatureMax() ? featureMax : state_.maxFeatureMax();
    state_.setSignalDiagnostics(state_.lastMicRms(), state_.lastMicPeak(), state_.maxMicRms(), state_.maxMicPeak(), rawMin, rawMax, maxRawMax, featureMin, featureMax, maxFeatureMax, state_.lastRawOutput(), maxRawOutput_);
    state_.setDiagnostics(queuedBlocks_, processedBlocks_, featureFrames_, inferenceRuns_, lastSamplesConsumed_, M5.Mic.isRecording());
    return runInference(features, error);
}

bool WakeController::runInference(const int8_t features[], String& error)
{
    TfLiteTensor* input = interpreter_->input(0);
    const uint8_t stride = input->dims->data[1];
    currentStrideStep_ = currentStrideStep_ % stride;

    int8_t* inputData = tflite::GetTensorData<int8_t>(input);
    memcpy(inputData + (kWakeFeatureSize * currentStrideStep_), features, kWakeFeatureSize);
    ++currentStrideStep_;

    if (currentStrideStep_ < stride) {
        return true;
    }

    currentStrideStep_ = 0;
    if (interpreter_->Invoke() != kTfLiteOk) {
        error = "WAKE_INFERENCE_FAILED";
        return false;
    }
    ++inferenceRuns_;

    const TfLiteTensor* output = interpreter_->output(0);
    const uint8_t rawOutput = output->data.uint8[0];
    if (rawOutput > maxRawOutput_) {
        maxRawOutput_ = rawOutput;
    }
    ++probabilityIndex_;
    if (probabilityIndex_ == kWakeSlidingWindowSize) {
        probabilityIndex_ = 0;
    }
    recentProbabilities_[probabilityIndex_] = rawOutput;
    state_.setSignalDiagnostics(state_.lastMicRms(), state_.lastMicPeak(), state_.maxMicRms(), state_.maxMicPeak(), state_.lastFeatureRawMin(), state_.lastFeatureRawMax(), state_.maxFeatureRawMax(), state_.lastFeatureMin(), state_.lastFeatureMax(), state_.maxFeatureMax(), rawOutput, maxRawOutput_);

    if (recentProbabilities_[probabilityIndex_] < probabilityCutoff()) {
        ++ignoreWindows_;
        if (ignoreWindows_ > 0) {
            ignoreWindows_ = 0;
        }
    }

    if (determineDetected()) {
        state_.captureDetectedDiagnostics();
        state_.setDetected(true);
        state_.setLastDetectedAtMs(millis());
        state_.setRunning(false);
        stopFrontend();
    }
    return true;
}

bool WakeController::determineDetected()
{
    uint32_t sum = 0;
    uint8_t maxProbability = 0;
    for (uint8_t probability : recentProbabilities_) {
        if (probability > maxProbability) {
            maxProbability = probability;
        }
        sum += probability;
    }

    const uint8_t average = sum / kWakeSlidingWindowSize;
    state_.setProbabilities(maxProbability, average);
    if (ignoreWindows_ < 0) {
        return false;
    }

    const bool detected = sum > (static_cast<uint32_t>(probabilityCutoff()) * kWakeSlidingWindowSize);
    if (detected) {
        resetProbabilities();
    }
    return detected;
}

void WakeController::resetProbabilities()
{
    memset(recentProbabilities_, 0, sizeof(recentProbabilities_));
    probabilityIndex_ = 0;
    currentStrideStep_ = 0;
    ignoreWindows_ = -kWakeMinSlicesBeforeDetection;
}

void WakeController::registerModelOps()
{
    opResolver_.AddCallOnce();
    opResolver_.AddVarHandle();
    opResolver_.AddReshape();
    opResolver_.AddReadVariable();
    opResolver_.AddStridedSlice();
    opResolver_.AddConcatenation();
    opResolver_.AddAssignVariable();
    opResolver_.AddConv2D();
    opResolver_.AddMul();
    opResolver_.AddAdd();
    opResolver_.AddMean();
    opResolver_.AddFullyConnected();
    opResolver_.AddLogistic();
    opResolver_.AddQuantize();
    opResolver_.AddDepthwiseConv2D();
    opResolver_.AddAveragePool2D();
    opResolver_.AddMaxPool2D();
    opResolver_.AddPad();
    opResolver_.AddPack();
    opResolver_.AddSplitV();
}

uint8_t* WakeController::allocateArena(size_t size)
{
    uint8_t* arena = static_cast<uint8_t*>(ps_malloc(size));
    if (arena == nullptr) {
        arena = static_cast<uint8_t*>(malloc(size));
    }
    return arena;
}

}  // namespace stackchan
