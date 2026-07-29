#include "SubscriptionGuard.h"

std::function<void(const std::string&)> SubscriptionGuard::wrap(std::function<void(const std::string&)> handler) {
    // Captured by value: the control block outlives the guard/owner if a call
    // is already in flight when the owner is destroyed.
    return [state = state_, handler = std::move(handler)](const std::string& payload) {
        std::lock_guard<std::mutex> lock(state->mutex);
        if (!state->alive) {
            return;
        }
        handler(payload);
    };
}

void SubscriptionGuard::close() {
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->alive = false;
}

SubscriptionGuard::~SubscriptionGuard() {
    close();
}
