#include "ThirdParty/doctest.h"

#include <vector>

#include "InProcessMessageBus.h"

TEST_SUITE("InProcessMessageBus::publish") {

TEST_CASE("a single subscriber receives exactly the published payload") {
    InProcessMessageBus bus;
    std::string received;
    bus.subscribe("game.1", [&](const std::string& payload) { received = payload; });

    bus.publish("game.1", "hello");

    CHECK(received == "hello");
}

TEST_CASE("publishing with zero subscribers is a no-op") {
    InProcessMessageBus bus;

    CHECK_NOTHROW(bus.publish("game.1", "hello"));
}

TEST_CASE("two subscribers on the same channel both receive the same message") {
    InProcessMessageBus bus;
    std::string first;
    std::string second;
    bus.subscribe("game.1", [&](const std::string& payload) { first = payload; });
    bus.subscribe("game.1", [&](const std::string& payload) { second = payload; });

    bus.publish("game.1", "hello");

    CHECK(first == "hello");
    CHECK(second == "hello");
}

TEST_CASE("a subscriber on one channel never fires for another channel") {
    InProcessMessageBus bus;
    bool fired = false;
    bus.subscribe("game.1", [&](const std::string&) { fired = true; });

    bus.publish("game.2", "hello");

    CHECK_FALSE(fired);
}

TEST_CASE("multiple publishes arrive at a subscriber in send order") {
    InProcessMessageBus bus;
    std::vector<std::string> received;
    bus.subscribe("game.1", [&](const std::string& payload) { received.push_back(payload); });

    bus.publish("game.1", "first");
    bus.publish("game.1", "second");
    bus.publish("game.1", "third");

    CHECK(received == std::vector<std::string>{ "first", "second", "third" });
}

TEST_CASE("an empty-string payload is delivered as-is") {
    InProcessMessageBus bus;
    std::string received = "unset";
    bus.subscribe("game.1", [&](const std::string& payload) { received = payload; });

    bus.publish("game.1", "");

    CHECK(received == "");
}

}
