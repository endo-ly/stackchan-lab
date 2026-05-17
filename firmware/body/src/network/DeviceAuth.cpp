#include "network/DeviceAuth.hpp"

namespace stackchan::network {

void DeviceAuth::setToken(const String& token)
{
    token_ = token;
}

bool DeviceAuth::enabled() const
{
    return token_.length() > 0;
}

bool DeviceAuth::authorize(WebServer& server) const
{
    if (!enabled()) {
        return true;
    }
    return server.header("X-StackChan-Token") == token_;
}

}  // namespace stackchan::network
