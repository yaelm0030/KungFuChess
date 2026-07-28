#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "MessageBus.h"

// Test-only fake: publish() calls matching handlers synchronously, in registration
// order, on the caller's thread. No unsubscribe, no backlog/replay.
class InProcessMessageBus : public MessageBus {
public:
    void publish(const std::string& channel, const std::string& payload) override {
        const auto it = handlers_.find(channel);
        if (it == handlers_.end()) {
            return;
        }
        for (const auto& handler : it->second) {
            handler(payload);
        }
    }

    void subscribe(const std::string& channel, std::function<void(const std::string&)> handler) override {
        handlers_[channel].push_back(std::move(handler));
    }

private:
    std::map<std::string, std::vector<std::function<void(const std::string&)>>> handlers_;
};
