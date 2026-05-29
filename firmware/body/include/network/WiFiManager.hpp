#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "network/NetworkState.hpp"

namespace stackchan::network {

struct WiFiConfig {
    String ssid;
    String password;
    String hostname = "stackchan-001";
    String authToken;
    String speechServicesUrl;
    bool wakeAutoStart = false;
};

class WiFiManager {
public:
    void begin();
    bool loadConfig(WiFiConfig& config);
    bool saveConfig(const WiFiConfig& config);
    bool saveSpeechServicesUrl(const String& url);
    bool saveDeviceConfig(const String& speechServicesUrl, bool wakeAutoStart);
    bool saveWakeAutoStart(bool enabled);
    bool clearConfig();
    bool connectSaved(uint32_t timeoutMs = 15000);
    bool connect(const WiFiConfig& config, uint32_t timeoutMs = 15000);
    void disconnect();
    void update();

    const WiFiConfig& config() const;
    const NetworkState& state() const;

private:
    void updateState();
    bool waitForConnection(uint32_t timeoutMs);

    Preferences preferences_;
    WiFiConfig config_;
    NetworkState state_;
};

}  // namespace stackchan::network
