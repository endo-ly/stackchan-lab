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
    void handleAudioStatus();
    void handleAudioVolume();
    void handleAudioStop();
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
};

}  // namespace stackchan::network
