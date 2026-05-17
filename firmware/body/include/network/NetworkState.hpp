#pragma once

#include <Arduino.h>

namespace stackchan::network {

struct NetworkState {
    bool enabled = true;
    bool connected = false;
    String ssid;
    String ip;
    String hostname = "stackchan-001";
    String macAddress;
    int32_t rssi = 0;
    bool httpServerRunning = false;
    bool authEnabled = false;
    uint32_t lastConnectedAt = 0;
    String lastError;
};

}  // namespace stackchan::network
