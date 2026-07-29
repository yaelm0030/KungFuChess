#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>

// Owned by a MessageBus subscriber to make its subscribe() handler safe to
// outlive it: wrap() returns a copy of the handler that becomes a no-op once
// close() has run, and close() blocks until any in-flight call finishes. This
// closes the race where a background reader thread (e.g. RedisMessageBus)
// still holds the subscriber's handler and delivers a message after the
// subscriber's own destructor has torn down its state.
class SubscriptionGuard {
public:
    // Wraps handler so any call after close() (or racing with it) becomes a safe
    // no-op instead of running handler.
    std::function<void(const std::string&)> wrap(std::function<void(const std::string&)> handler);

    // Blocks until any in-flight wrapped() call finishes, then makes all future
    // wrapped() calls no-ops. Idempotent. Safe to call from the destructor.
    void close();

    ~SubscriptionGuard(); // Calls close(); belt-and-suspenders if a subscriber forgets to.

private:
    struct State {
        std::mutex mutex;
        bool alive = true;
    };

    std::shared_ptr<State> state_ = std::make_shared<State>();
};
