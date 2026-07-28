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

TEST_SUITE("WebSocketGateway::color assignment") {

TEST_CASE("the first connection becomes White: it can select White pieces but not Black ones") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port());
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("click 50 50"); // -> cell (0,0), the black rook: not White's to select
    REQUIRE(client.wait_for_messages(4, kWaitTimeout));
    nlohmann::json ignored_json = nlohmann::json::parse(client.last_message());
    CHECK(ignored_json["selected_w"].is_null());
    CHECK(ignored_json["selected_b"].is_null());

    client.send("click 50 250"); // -> cell (0,2), the white rook
    REQUIRE(client.wait_for_messages(7, kWaitTimeout));
    nlohmann::json selected_json = nlohmann::json::parse(client.last_message());
    REQUIRE(selected_json["selected_w"].is_object());
    CHECK(selected_json["selected_w"]["x"] == 0);
    CHECK(selected_json["selected_w"]["y"] == 2);
    CHECK(selected_json["selected_b"].is_null());

    client.close();
}

TEST_CASE("the second connection becomes Black: it can select Black pieces but not White ones") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client_a(sg.gateway.port()); // becomes White
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    TestClient client_b(sg.gateway.port()); // becomes Black
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    client_b.send("click 50 250"); // -> cell (0,2), the white rook: not Black's to select
    REQUIRE(client_b.wait_for_messages(4, kWaitTimeout));
    nlohmann::json ignored_json = nlohmann::json::parse(client_b.last_message());
    CHECK(ignored_json["selected_w"].is_null());
    CHECK(ignored_json["selected_b"].is_null());

    client_b.send("click 50 50"); // -> cell (0,0), the black rook
    REQUIRE(client_b.wait_for_messages(7, kWaitTimeout));
    nlohmann::json selected_json = nlohmann::json::parse(client_b.last_message());
    REQUIRE(selected_json["selected_b"].is_object());
    CHECK(selected_json["selected_b"]["x"] == 0);
    CHECK(selected_json["selected_b"]["y"] == 0);
    CHECK(selected_json["selected_w"].is_null());

    client_a.close();
    client_b.close();
}

TEST_CASE("a third connection is a spectator: it keeps receiving broadcasts but its clicks never select anything") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client_a(sg.gateway.port()); // White
    TestClient client_b(sg.gateway.port()); // Black
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    TestClient client_c(sg.gateway.port()); // spectator
    REQUIRE(client_c.wait_for_messages(1, kWaitTimeout));

    client_c.send("click 50 250"); // would select the white rook for an actual player

    // client_c must keep receiving broadcasts well past its dropped click,
    // proving the gateway didn't crash and didn't stall.
    REQUIRE(client_c.wait_for_messages(6, kWaitTimeout));
    nlohmann::json json = nlohmann::json::parse(client_c.last_message());
    CHECK(json["selected_w"].is_null());
    CHECK(json["selected_b"].is_null());

    client_a.close();
    client_b.close();
    client_c.close();
}

TEST_CASE("a disconnect frees its color slot for the next connection") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client_a(sg.gateway.port()); // White
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    TestClient client_b(sg.gateway.port()); // Black
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    client_a.close();

    // Give the gateway's close handler time to free the White slot before the
    // next connection arrives, using client_b's continued broadcasts as a clock.
    REQUIRE(client_b.wait_for_messages(8, kWaitTimeout));

    TestClient client_c(sg.gateway.port());
    REQUIRE(client_c.wait_for_messages(1, kWaitTimeout));

    client_c.send("click 50 250"); // -> cell (0,2), the white rook: only selectable if client_c is now White
    REQUIRE(client_c.wait_for_messages(4, kWaitTimeout));
    nlohmann::json json = nlohmann::json::parse(client_c.last_message());
    REQUIRE(json["selected_w"].is_object());
    CHECK(json["selected_w"]["x"] == 0);
    CHECK(json["selected_w"]["y"] == 2);

    client_b.close();
    client_c.close();
}

}
