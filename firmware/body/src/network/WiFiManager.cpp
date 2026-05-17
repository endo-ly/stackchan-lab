#include "network/WiFiManager.hpp"

#include <WiFi.h>

namespace stackchan::network {
namespace {

constexpr const char* kNamespace = "stackchan";
constexpr const char* kSsidKey = "ssid";
constexpr const char* kPasswordKey = "password";
constexpr const char* kHostnameKey = "hostname";
constexpr const char* kAuthTokenKey = "auth_token";

}  // namespace

void WiFiManager::begin()
{
    WiFi.mode(WIFI_STA);
    state_.macAddress = WiFi.macAddress();
}

bool WiFiManager::loadConfig(WiFiConfig& config)
{
    if (!preferences_.begin(kNamespace, true)) {
        state_.lastError = "preferences_open_failed";
        return false;
    }
    config.ssid = preferences_.getString(kSsidKey, "");
    config.password = preferences_.getString(kPasswordKey, "");
    config.hostname = preferences_.getString(kHostnameKey, "stackchan-001");
    config.authToken = preferences_.getString(kAuthTokenKey, "");
    preferences_.end();
    return config.ssid.length() > 0;
}

bool WiFiManager::saveConfig(const WiFiConfig& config)
{
    if (config.ssid.length() == 0 || config.hostname.length() == 0) {
        state_.lastError = "invalid_config";
        return false;
    }
    if (!preferences_.begin(kNamespace, false)) {
        state_.lastError = "preferences_open_failed";
        return false;
    }
    const bool ok = preferences_.putString(kSsidKey, config.ssid) > 0
        && preferences_.putString(kPasswordKey, config.password) == config.password.length()
        && preferences_.putString(kHostnameKey, config.hostname) > 0
        && preferences_.putString(kAuthTokenKey, config.authToken) == config.authToken.length();
    preferences_.end();
    if (ok) {
        config_ = config;
        state_.hostname = config.hostname;
        state_.authEnabled = config.authToken.length() > 0;
        state_.lastError = "";
    } else {
        state_.lastError = "save_failed";
    }
    return ok;
}

bool WiFiManager::clearConfig()
{
    disconnect();
    if (!preferences_.begin(kNamespace, false)) {
        state_.lastError = "preferences_open_failed";
        return false;
    }
    const bool ok = preferences_.clear();
    preferences_.end();
    config_ = WiFiConfig{};
    state_ = NetworkState{};
    state_.macAddress = WiFi.macAddress();
    state_.lastError = ok ? "" : "clear_failed";
    return ok;
}

bool WiFiManager::connectSaved(uint32_t timeoutMs)
{
    WiFiConfig loaded;
    if (!loadConfig(loaded)) {
        state_.lastError = "wifi_config_missing";
        updateState();
        return false;
    }
    return connect(loaded, timeoutMs);
}

bool WiFiManager::connect(const WiFiConfig& config, uint32_t timeoutMs)
{
    config_ = config;
    state_.hostname = config.hostname;
    state_.ssid = config.ssid;
    state_.authEnabled = config.authToken.length() > 0;
    state_.lastError = "";

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(config.hostname.c_str());
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    const bool ok = waitForConnection(timeoutMs);
    updateState();
    if (!ok) {
        state_.lastError = "connect_timeout";
    }
    return ok;
}

void WiFiManager::disconnect()
{
    WiFi.disconnect(true, false);
    updateState();
}

void WiFiManager::update()
{
    updateState();
}

const WiFiConfig& WiFiManager::config() const
{
    return config_;
}

const NetworkState& WiFiManager::state() const
{
    return state_;
}

bool WiFiManager::waitForConnection(uint32_t timeoutMs)
{
    const uint32_t startedAt = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startedAt >= timeoutMs) {
            return false;
        }
        delay(100);
    }
    state_.lastConnectedAt = millis();
    return true;
}

void WiFiManager::updateState()
{
    state_.connected = WiFi.status() == WL_CONNECTED;
    state_.ssid = config_.ssid;
    state_.hostname = config_.hostname.length() > 0 ? config_.hostname : "stackchan-001";
    state_.macAddress = WiFi.macAddress();
    state_.ip = state_.connected ? WiFi.localIP().toString() : "";
    state_.rssi = state_.connected ? WiFi.RSSI() : 0;
    state_.authEnabled = config_.authToken.length() > 0;
}

}  // namespace stackchan::network
