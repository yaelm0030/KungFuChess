#include "ThirdParty/doctest.h"

#include "WebSocketGateway.h"
#include "../MessageBus/InProcessMessageBus.h"
#include "../UserRepository/FakeUserRepository.h"

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

}
