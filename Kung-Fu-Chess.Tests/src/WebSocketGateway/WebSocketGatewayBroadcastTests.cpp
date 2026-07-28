#include "ThirdParty/doctest.h"

#include <chrono>

#include <nlohmann/json.hpp>

#include "Parser.h"
#include "ShardAndGateway.h"
#include "../GameServer/TestClient.h"
#include "../UserRepository/FakeUserRepository.h"

namespace {

Board make_board() {
    return Parser::parse_board({
        "bR bN .",
        ".  .  .",
        "wR .  wN",
    });
}

constexpr std::chrono::milliseconds kWaitTimeout{ 2000 };

} // namespace

TEST_SUITE("WebSocketGateway::broadcast") {

TEST_CASE("a connected client receives a JSON broadcast after start") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port());
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    nlohmann::json json = nlohmann::json::parse(client.last_message());
    CHECK(json["board_width"] == 3);
    CHECK(json["board_height"] == 3);
    CHECK(json["pieces"].size() == 4);

    client.close();
}

TEST_CASE("two independently connected clients both receive the same broadcast content") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client_a(sg.gateway.port());
    TestClient client_b(sg.gateway.port());
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    CHECK(client_a.last_message() == client_b.last_message());

    client_a.close();
    client_b.close();
}

TEST_CASE("a client that disconnects mid-session is dropped without affecting the other client's future broadcasts") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client_a(sg.gateway.port());
    TestClient client_b(sg.gateway.port());
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    client_a.close();

    // client_b must keep receiving broadcasts well past the point client_a
    // disconnected, proving the gateway didn't crash and didn't stall.
    REQUIRE(client_b.wait_for_messages(5, kWaitTimeout));

    client_b.close();
}

}
