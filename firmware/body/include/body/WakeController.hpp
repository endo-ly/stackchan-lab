#pragma once

#include <Arduino.h>
#include <MicroTensorFlowLite.h>
#include <frontend.h>
#include <frontend_util.h>
#include <tensorflow/lite/micro/micro_allocator.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/micro_resource_variable.h>
#include <tensorflow/lite/schema/schema_generated.h>

#include "body/WakeModelData.hpp"
#include "body/WakeState.hpp"

namespace stackchan {

class WakeController {
public:
    void begin();
    void update();

    bool start(String& error);
    void stop();

    const WakeState& getState() const;

private:
    bool loadModel(String& error);
    bool startFrontend(String& error);
    void stopFrontend();
    void queueMicChunk();
    void processMicChunk();
    bool processFeatures(const int16_t* samples, size_t sampleCount, String& error);
    bool runInference(const int8_t features[], String& error);
    bool determineDetected();
    void resetProbabilities();
    void registerModelOps();
    static uint8_t* allocateArena(size_t size);

    WakeState state_;
    struct FrontendConfig frontendConfig_ = {};
    struct FrontendState frontendState_ = {};
    tflite::MicroMutableOpResolver<20> opResolver_;
    const tflite::Model* model_ = nullptr;
    uint8_t* tensorArena_ = nullptr;
    uint8_t* variableArena_ = nullptr;
    tflite::MicroAllocator* variableAllocator_ = nullptr;
    tflite::MicroResourceVariables* resourceVariables_ = nullptr;
    tflite::MicroInterpreter* interpreter_ = nullptr;
    bool frontendReady_ = false;
    bool micChunkPending_ = false;
    uint8_t currentStrideStep_ = 0;
    uint8_t recentProbabilities_[kWakeSlidingWindowSize] = {};
    size_t probabilityIndex_ = 0;
    int16_t ignoreWindows_ = -kWakeMinSlicesBeforeDetection;
    static constexpr size_t kMicBlockSamples = 160;
    static constexpr size_t kMicBlocksPerChunk = 16;
    int16_t micChunk_[kMicBlockSamples * kMicBlocksPerChunk] = {};
    uint32_t queuedBlocks_ = 0;
    uint32_t processedBlocks_ = 0;
    uint32_t featureFrames_ = 0;
    uint32_t inferenceRuns_ = 0;
    uint32_t lastSamplesConsumed_ = 0;
    uint8_t maxRawOutput_ = 0;
};

}  // namespace stackchan
