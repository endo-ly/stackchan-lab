#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "body/BodyController.hpp"
#include "network/DeviceAuth.hpp"
#include "network/WiFiManager.hpp"

namespace stackchan::network {

class HttpServerController {
public:
    HttpServerController(BodyController& body, WiFiManager& wifi, DeviceAuth& auth);

    void begin();
    void update();
    bool isRunning() const;

private:
    void registerRoutes();
    void processWakeDetection();
    void restartWakeDetection();
    bool requireAuth();
    void sendOk(const String& dataJson);
    void sendError(int status, const char* code, const String& message);
    String readBody();

    void handleHealth();
    void handleVersion();
    void handleStatus();
    void handleCapabilities();
    void handleFace();
    void handleLed();
    void handlePose();
    void handleMove();
    void handleReset();
    void handlePlayWav();
    void handlePlayWavBody();
    void handleAudioStatus();
    void handleAudioVolume();
    void handleAudioStop();
    void handleMicStatus();
    void handleMicRecord();
    void handleWakeStatus();
    void handleWakeStart();
    void handleWakeStop();
    void handleEvents();
    void handleLatestEvent();
    void handleClearEvents();
    void handleWifiStatus();
    void handleWifiConnect();
    void handleWifiClear();
    void handleCameraSnapshot();

    BodyController& body_;
    WiFiManager& wifi_;
    DeviceAuth& auth_;
    WebServer server_;
    bool running_ = false;
    bool wakeUploadInProgress_ = false;
    uint32_t handledWakeDetectedAtMs_ = 0;
    uint32_t lastWakeUploadAtMs_ = 0;
    int lastWakeUploadHttpStatus_ = 0;
    String lastWakeUploadError_;
    String lastWakeSpeechResponse_;
    uint8_t* wavBodyBuffer_ = nullptr;
    size_t wavBodySize_ = 0;
    String wavBodyError_;
};

}  // namespace stackchan::network
