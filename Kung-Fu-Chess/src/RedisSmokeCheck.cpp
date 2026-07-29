#include "RedisSmokeCheck.h"

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "RedisMessageBus.h"

std::string redis_uri() {
    // std::getenv is flagged C4996 by MSVC in favor of getenv_s; the codebase has no
    // blanket _CRT_SECURE_NO_WARNINGS, so suppress locally at this call site only.
#pragma warning(push)
#pragma warning(disable : 4996)
    const char* env_value = std::getenv(kRedisUriEnvVar);
#pragma warning(pop)
    if (env_value != nullptr && env_value[0] != '\0') {
        return env_value;
    }
    return kRedisConnectionUri;
}

namespace {

constexpr const char* kSmokeTestChannel = "__redis_smoke_check__";
constexpr const char* kSmokeTestPayload = "hello redis";
constexpr auto kWaitTimeout = std::chrono::seconds(5);

constexpr const char* kReentrantFirstChannel = "__redis_smoke_check_reentrant_first__";
constexpr const char* kReentrantSecondChannel = "__redis_smoke_check_reentrant_second__";
constexpr const char* kReentrantSecondPayload = "hello reentrant redis";

// Publishes a known payload from one connection ("Gateway") and confirms a second
// connection ("Shard"), subscribed to the same channel, actually receives it.
int run_pubsub_check() {
    try {
        const std::string uri = redis_uri();
        RedisMessageBus shard_bus(uri);
        RedisMessageBus gateway_bus(uri);

        std::mutex mutex;
        std::condition_variable cv;
        std::string received_payload;
        bool received = false;

        shard_bus.subscribe(kSmokeTestChannel, [&](const std::string& payload) {
            std::lock_guard<std::mutex> lock(mutex);
            received_payload = payload;
            received = true;
            cv.notify_all();
        });

        // Gives the SUBSCRIBE issued above time to register before publishing.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        gateway_bus.publish(kSmokeTestChannel, kSmokeTestPayload);

        std::unique_lock<std::mutex> lock(mutex);
        if (!cv.wait_for(lock, kWaitTimeout, [&] { return received; })) {
            std::cout << "redis smoke check FAILED: no message received within timeout\n";
            return 1;
        }
        if (received_payload != kSmokeTestPayload) {
            std::cout << "redis smoke check FAILED: unexpected payload \"" << received_payload << "\"\n";
            return 1;
        }

        std::cout << "redis smoke check OK: pub/sub round-trip via " << uri << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "redis smoke check FAILED: " << e.what() << "\n";
        return 1;
    }
}

// Regression check for the reentrant-subscribe deadlock: a handler running on the reader
// thread calls subscribe() on a previously-unsubscribed channel. Confirms this neither
// hangs (the original bug) nor is silently dropped (the second channel must actually
// receive its own publish afterward).
int run_reentrant_subscribe_check() {
    try {
        const std::string uri = redis_uri();
        RedisMessageBus shard_bus(uri);
        RedisMessageBus gateway_bus(uri);

        std::mutex mutex;
        std::condition_variable cv;
        std::string second_payload;
        bool second_received = false;

        shard_bus.subscribe(kReentrantFirstChannel, [&](const std::string&) {
            shard_bus.subscribe(kReentrantSecondChannel, [&](const std::string& payload) {
                std::lock_guard<std::mutex> lock(mutex);
                second_payload = payload;
                second_received = true;
                cv.notify_all();
            });
        });

        // Gives the SUBSCRIBE issued above time to register before publishing.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        gateway_bus.publish(kReentrantFirstChannel, kSmokeTestPayload);

        // Gives the reentrant SUBSCRIBE (applied on the reader thread's next loop
        // iteration) time to register before publishing to the second channel.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        gateway_bus.publish(kReentrantSecondChannel, kReentrantSecondPayload);

        std::unique_lock<std::mutex> lock(mutex);
        if (!cv.wait_for(lock, kWaitTimeout, [&] { return second_received; })) {
            std::cout << "redis reentrant subscribe check FAILED: no message received within timeout\n";
            return 1;
        }
        if (second_payload != kReentrantSecondPayload) {
            std::cout << "redis reentrant subscribe check FAILED: unexpected payload \"" << second_payload << "\"\n";
            return 1;
        }

        std::cout << "redis reentrant subscribe check OK: reentrant subscribe from a handler took effect\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "redis reentrant subscribe check FAILED: " << e.what() << "\n";
        return 1;
    }
}

// Confirms construction fails fast (throws) instead of hanging when Redis is unreachable.
int run_unreachable_check() {
    try {
        RedisMessageBus{ "tcp://127.0.0.1:1" };
        std::cout << "redis unreachable check FAILED: construction did not throw\n";
        return 1;
    } catch (const std::exception&) {
        std::cout << "redis unreachable check OK: construction threw as expected\n";
        return 0;
    }
}

} // namespace

int run_redis_smoke_check() {
    int pubsub_result = run_pubsub_check();
    if (pubsub_result != 0) {
        return pubsub_result;
    }
    int reentrant_result = run_reentrant_subscribe_check();
    if (reentrant_result != 0) {
        return reentrant_result;
    }
    return run_unreachable_check();
}
