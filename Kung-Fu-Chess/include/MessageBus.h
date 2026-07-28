#pragma once

#include <functional>
#include <string>

// Publish/subscribe abstraction between the WebSocket Gateway and Game Shard processes.
class MessageBus {
public:
    virtual ~MessageBus() = default;
    virtual void publish(const std::string& channel, const std::string& payload) = 0;
    // handler may be invoked on any thread (a future Redis impl reads on a background thread); must be internally thread-safe.
    virtual void subscribe(const std::string& channel, std::function<void(const std::string&)> handler) = 0;
};
