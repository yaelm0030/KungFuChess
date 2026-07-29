#include "RedisMessageBus.h"

#include <chrono>

namespace {

constexpr std::chrono::milliseconds kSubscriberSocketTimeout{ 200 };

// A fresh connection with a short socket_timeout, so run_reader_loop's consume()
// call returns periodically even when idle instead of blocking forever.
sw::redis::Subscriber make_subscriber(const std::string& uri) {
    sw::redis::ConnectionOptions options = sw::redis::Uri(uri).connection_options();
    options.socket_timeout = kSubscriberSocketTimeout;
    return sw::redis::Redis(options).subscriber();
}

} // namespace

RedisMessageBus::RedisMessageBus(const std::string& uri) : redis_(uri), subscriber_(make_subscriber(uri)) {
    redis_.ping();

    subscriber_.on_message([this](std::string channel, std::string payload) { dispatch(channel, payload); });
    reader_thread_ = std::thread(&RedisMessageBus::run_reader_loop, this);
}

RedisMessageBus::~RedisMessageBus() {
    running_ = false;
    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }
}

void RedisMessageBus::publish(const std::string& channel, const std::string& payload) {
    redis_.publish(channel, payload);
}

void RedisMessageBus::subscribe(const std::string& channel, std::function<void(const std::string&)> handler) {
    bool first_handler = false;
    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        std::vector<std::function<void(const std::string&)>>& channel_handlers = handlers_[channel];
        first_handler = channel_handlers.empty();
        channel_handlers.push_back(std::move(handler));
    }

    if (!first_handler) {
        return;
    }

    if (std::this_thread::get_id() == reader_thread_.get_id()) {
        // Called reentrantly from inside a handler running on the reader thread,
        // which already holds subscriber_mutex_ for the duration of consume().
        // Locking it again here would self-deadlock, so queue the channel instead;
        // run_reader_loop() applies it between consume() calls.
        std::lock_guard<std::mutex> lock(pending_subscriptions_mutex_);
        pending_subscriptions_.push_back(channel);
        return;
    }

    std::lock_guard<std::mutex> lock(subscriber_mutex_);
    subscriber_.subscribe(channel);
}

void RedisMessageBus::dispatch(const std::string& channel, const std::string& payload) {
    std::vector<std::function<void(const std::string&)>> handlers_snapshot;
    {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        const auto it = handlers_.find(channel);
        if (it == handlers_.end()) {
            return;
        }
        handlers_snapshot = it->second;
    }

    // Handlers run without handlers_mutex_ held so a handler that reentrantly
    // calls subscribe() (even for this channel) doesn't self-deadlock or
    // invalidate this vector mid-iteration.
    for (const auto& handler : handlers_snapshot) {
        handler(payload);
    }
}

void RedisMessageBus::apply_pending_subscriptions() {
    std::vector<std::string> channels;
    {
        std::lock_guard<std::mutex> lock(pending_subscriptions_mutex_);
        channels.swap(pending_subscriptions_);
    }
    for (const auto& channel : channels) {
        subscriber_.subscribe(channel);
    }
}

void RedisMessageBus::run_reader_loop() {
    while (running_) {
        try {
            std::lock_guard<std::mutex> lock(subscriber_mutex_);
            apply_pending_subscriptions();
            subscriber_.consume();
        } catch (const sw::redis::TimeoutError&) {
            // Expected: socket_timeout elapsed with no message; loop back and re-check running_.
        } catch (const std::exception&) {
            // Real error (e.g. connection dropped); stop instead of spinning on the same failure.
            running_ = false;
        }
    }
}
