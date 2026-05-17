#pragma once

#include <Arduino.h>

#include "body/BodyController.hpp"
#include "network/DeviceAuth.hpp"
#include "network/HttpServerController.hpp"
#include "network/WiFiManager.hpp"

namespace stackchan::network {

class NetworkController {
public:
    explicit NetworkController(BodyController& body);

    void begin();
    void update();
    bool connect();
    bool connect(const WiFiConfig& config);
    bool clearConfig();

    WiFiManager& wifi();
    const WiFiManager& wifi() const;
    const NetworkState& getState() const;

private:
    BodyController& body_;
    WiFiManager wifi_;
    DeviceAuth auth_;
    HttpServerController http_;
};

}  // namespace stackchan::network
