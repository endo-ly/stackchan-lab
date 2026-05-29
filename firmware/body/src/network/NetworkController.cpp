#include "network/NetworkController.hpp"

#include <ESPmDNS.h>

namespace stackchan::network {
namespace {

void startWakeIfConfigured(BodyController& body, const WiFiConfig& config)
{
    if (!config.wakeAutoStart) {
        return;
    }
    if (config.speechServicesUrl.length() == 0) {
        Serial.println("[NET][WARN] wake auto-start skipped: speechServicesUrl is not configured");
        return;
    }

    String error;
    if (body.startWake(error)) {
        Serial.println("[NET] wake auto-started");
        return;
    }
    Serial.print("[NET][WARN] wake auto-start failed: ");
    Serial.println(error);
}

}  // namespace

NetworkController::NetworkController(BodyController& body)
    : body_(body)
    , http_(body, wifi_, auth_)
{
}

void NetworkController::begin()
{
    Serial.println("[NET] begin");
    wifi_.begin();
    WiFiConfig config;
    if (!wifi_.loadConfig(config)) {
        Serial.println("[NET] saved Wi-Fi config not found");
        return;
    }
    auth_.setToken(config.authToken);
    if (!wifi_.connect(config)) {
        Serial.print("[NET][WARN] Wi-Fi connect failed: ");
        Serial.println(wifi_.state().lastError);
        return;
    }
    if (MDNS.begin(config.hostname.c_str())) {
        Serial.print("[NET] mDNS hostname=");
        Serial.println(config.hostname);
    }
    http_.begin();
    startWakeIfConfigured(body_, wifi_.config());
}

void NetworkController::update()
{
    wifi_.update();
    http_.update();
}

bool NetworkController::connect()
{
    WiFiConfig config;
    if (!wifi_.loadConfig(config)) {
        return false;
    }
    auth_.setToken(config.authToken);
    const bool ok = wifi_.connect(config);
    if (ok && !http_.isRunning()) {
        MDNS.begin(config.hostname.c_str());
        http_.begin();
        startWakeIfConfigured(body_, wifi_.config());
    }
    return ok;
}

bool NetworkController::connect(const WiFiConfig& config)
{
    auth_.setToken(config.authToken);
    const bool ok = wifi_.connect(config);
    if (ok && !http_.isRunning()) {
        MDNS.begin(config.hostname.c_str());
        http_.begin();
        startWakeIfConfigured(body_, wifi_.config());
    }
    return ok;
}

bool NetworkController::clearConfig()
{
    auth_.setToken("");
    return wifi_.clearConfig();
}

WiFiManager& NetworkController::wifi()
{
    return wifi_;
}

const WiFiManager& NetworkController::wifi() const
{
    return wifi_;
}

const NetworkState& NetworkController::getState() const
{
    return wifi_.state();
}

}  // namespace stackchan::network
