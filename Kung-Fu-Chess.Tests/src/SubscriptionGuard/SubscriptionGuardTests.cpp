#include "ThirdParty/doctest.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "SubscriptionGuard.h"

TEST_SUITE("SubscriptionGuard") {

TEST_CASE("a wrapped handler still runs normally before close()") {
    SubscriptionGuard guard;
    std::string received;
    auto wrapped = guard.wrap([&](const std::string& payload) { received = payload; });

    wrapped("hello");

    CHECK(received == "hello");
}

TEST_CASE("after close(), calling the wrapped handler is a no-op") {
    SubscriptionGuard guard;
    bool called = false;
    auto wrapped = guard.wrap([&](const std::string&) { called = true; });

    guard.close();
    wrapped("hello");

    CHECK_FALSE(called);
}

TEST_CASE("close() is idempotent: calling it twice does not throw or hang, and wrapped calls after remain no-ops") {
    SubscriptionGuard guard;
    bool called = false;
    auto wrapped = guard.wrap([&](const std::string&) { called = true; });

    guard.close();
    CHECK_NOTHROW(guard.close());
    wrapped("hello");

    CHECK_FALSE(called);
}

TEST_CASE("the destructor closes the guard even if close() was never called explicitly") {
    bool called = false;
    std::function<void(const std::string&)> wrapped;
    {
        SubscriptionGuard guard;
        wrapped = guard.wrap([&](const std::string&) { called = true; });
    }

    CHECK_NOTHROW(wrapped("hello"));
    CHECK_FALSE(called);
}

TEST_CASE("close() blocks until an in-flight wrapped call finishes") {
    SubscriptionGuard guard;
    std::atomic<bool> started{ false };
    std::vector<std::string> sequence;

    auto wrapped = guard.wrap([&](const std::string&) {
        sequence.push_back("started");
        started = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        sequence.push_back("finished");
    });

    std::thread caller([&]() { wrapped("hello"); });

    while (!started) {
        std::this_thread::yield();
    }
    guard.close();

    REQUIRE(sequence.size() == 2);
    CHECK(sequence[0] == "started");
    CHECK(sequence[1] == "finished");

    caller.join();
}

}
