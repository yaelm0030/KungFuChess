#include "ThirdParty/doctest.h"

#include <chrono>
#include <future>
#include <memory>
#include <thread>

#include "WebSocketGateway.h"
#include "../MessageBus/InProcessMessageBus.h"
#include "../UserRepository/FakeUserRepository.h"

namespace {

// A regression here is "stop() hangs forever", so stop() runs on a helper thread and
// the test waits on its future with a timeout instead of joining it directly. On
// timeout the helper thread is detached rather than joined, so gateway/stopped are
// captured by shared_ptr (not by reference) to stay alive for it: a reference into the
// test's stack frame would dangle the moment the TEST_CASE returns and its locals are
// destructed, racing the still-running detached thread. A confirmed regression leaks
// the gateway deliberately instead of risking that use-after-free.
bool stop_completes_within(std::shared_ptr<WebSocketGateway> gateway, std::chrono::milliseconds timeout) {
    auto stopped = std::make_shared<std::promise<void>>();
    std::future<void> stopped_future = stopped->get_future();
    std::thread stopper([gateway, stopped]() {
        gateway->stop();
        stopped->set_value();
    });
    bool completed = stopped_future.wait_for(timeout) == std::future_status::ready;
    if (completed) {
        stopper.join();
    } else {
        stopper.detach();
    }
    return completed;
}

} // namespace

TEST_SUITE("WebSocketGateway::start/stop") {

TEST_CASE("start then stop returns promptly without hanging or crashing") {
    InProcessMessageBus bus;
    FakeUserRepository repository;
    WebSocketGateway gateway(bus, repository);

    gateway.start(0);
    gateway.stop();

    CHECK(true);
}

TEST_CASE("stop is idempotent when called twice in a row") {
    InProcessMessageBus bus;
    FakeUserRepository repository;
    WebSocketGateway gateway(bus, repository);

    gateway.start(0);
    gateway.stop();
    gateway.stop();

    CHECK(true);
}

TEST_CASE("a gateway that goes out of scope while still running tears down cleanly") {
    {
        InProcessMessageBus bus;
        FakeUserRepository repository;
        WebSocketGateway gateway(bus, repository);
        gateway.start(0);
    }

    CHECK(true);
}

TEST_CASE("stop returns within a bounded time after the io thread has gone idle") {
    InProcessMessageBus bus;
    FakeUserRepository repository;
    auto gateway = std::make_shared<WebSocketGateway>(bus, repository);

    gateway->start(0);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    CHECK(stop_completes_within(gateway, std::chrono::seconds(5)));
}

TEST_CASE("stop returns within a bounded time on repeated idle-then-stop cycles") {
    for (int i = 0; i < 10; ++i) {
        InProcessMessageBus bus;
        FakeUserRepository repository;
        auto gateway = std::make_shared<WebSocketGateway>(bus, repository);

        gateway->start(0);
        std::this_thread::sleep_for(std::chrono::seconds(2));

        CHECK(stop_completes_within(gateway, std::chrono::seconds(5)));
    }
}

}
