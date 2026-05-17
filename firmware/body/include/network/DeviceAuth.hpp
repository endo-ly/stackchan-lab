#pragma once

#include <Arduino.h>
#include <WebServer.h>

namespace stackchan::network {

class DeviceAuth {
public:
    void setToken(const String& token);
    bool enabled() const;
    bool authorize(WebServer& server) const;

private:
    String token_;
};

}  // namespace stackchan::network
