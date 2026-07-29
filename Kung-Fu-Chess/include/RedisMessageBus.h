#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Silences an MSVC C4200 warning from hiredis's vendor headers.
#pragma warning(push)
#pragma warning(disable : 4200)
#include <sw/redis++/redis++.h>
#pragma warning(pop)

#include "MessageBus.h"

// Redis Pub/Sub-backed MessageBus: publish() goes through a pooled connection
// (redis_), subscribe() fans out to handlers on a dedicated background reader
// thread that drains a second, subscription-only connection (subscriber_).
class RedisMessageBus : public MessageBus {
public:
    // Connects and PINGs immediately; throws if Redis is unreachable. No retry —
    // construction either succeeds or the caller's startup fails fast.
    explicit RedisMessageBus(const std::string& uri);
    ~RedisMessageBus() override;

    void publish(const std::string& channel, const std::string& payload) override;
    // Registers handler for channel; issues the underlying SUBSCRIBE only the first
    // time a channel gets a handler. Handler runs on the internal reader thread.
    void subscribe(const std::string& channel, std::function<void(const std::string&)> handler) override;

private:
    void run_reader_loop();
    void dispatch(const std::string& channel, const std::string& payload);
    // Issues SUBSCRIBE for every channel queued by a reentrant subscribe() call.
    // Called by run_reader_loop() only, which already holds subscriber_mutex_.
    void apply_pending_subscriptions();

    sw::redis::Redis redis_;           // publish path; pool-backed, safe for concurrent use
    sw::redis::Subscriber subscriber_; // dedicated pub/sub connection, short socket_timeout

    std::mutex handlers_mutex_; // guards handlers_
    std::map<std::string, std::vector<std::function<void(const std::string&)>>> handlers_;

    std::mutex subscriber_mutex_; // Subscriber isn't thread-safe; guards subscriber_ access
    std::thread reader_thread_;
    std::atomic<bool> running_{ true };

    // Channels awaiting SUBSCRIBE because subscribe() was called reentrantly from a
    // handler on the reader thread (which already holds subscriber_mutex_ there).
    std::mutex pending_subscriptions_mutex_;
    std::vector<std::string> pending_subscriptions_;
};
