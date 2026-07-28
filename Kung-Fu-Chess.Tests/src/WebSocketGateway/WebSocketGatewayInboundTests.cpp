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

TEST_SUITE("WebSocketGateway::inbound") {

TEST_CASE("a client's click command selects the clicked piece, reflected in a subsequent broadcast") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port());
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("click 50 250"); // -> cell (0,2), the white rook

    // A few ticks' worth of headroom for the command to be applied and
    // picked up by a broadcast, bounded overall by kWaitTimeout.
    REQUIRE(client.wait_for_messages(4, kWaitTimeout));
    nlohmann::json json = nlohmann::json::parse(client.last_message());
    REQUIRE(json["selected_w"].is_object());
    CHECK(json["selected_w"]["x"] == 0);
    CHECK(json["selected_w"]["y"] == 2);

    client.close();
}

TEST_CASE("a malformed client message does not crash the gateway or disrupt other clients") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client_a(sg.gateway.port());
    TestClient client_b(sg.gateway.port());
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    client_a.send("this is not a valid command");
    client_a.send("click x");   // wrong arg count
    client_a.send("click a b"); // non-numeric args; stoi throws and is caught

    // client_b must keep receiving broadcasts well past the garbage sends,
    // proving the gateway didn't crash and didn't stall.
    REQUIRE(client_b.wait_for_messages(5, kWaitTimeout));

    client_a.close();
    client_b.close();
}

}
