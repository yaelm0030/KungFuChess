#include "ThirdParty/doctest.h"

#include <chrono>

#include <nlohmann/json.hpp>

#include "GameServer.h"
#include "Parser.h"
#include "TestClient.h"

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

TEST_SUITE("GameServer::inbound") {

TEST_CASE("a client's click command selects the clicked piece, reflected in a subsequent broadcast") {
    GameServer server(make_board());
    server.start(0);

    TestClient client(server.port());
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("click 50 50"); // -> cell (0,0), the black rook

    // A few ticks' worth of headroom for the command to be applied and
    // picked up by a broadcast, bounded overall by kWaitTimeout.
    REQUIRE(client.wait_for_messages(4, kWaitTimeout));
    nlohmann::json json = nlohmann::json::parse(client.last_message());
    REQUIRE(json["selected"].is_object());
    CHECK(json["selected"]["x"] == 0);
    CHECK(json["selected"]["y"] == 0);

    client.close();
    server.stop();
}

TEST_CASE("a malformed client message does not crash the server or disrupt other clients") {
    GameServer server(make_board());
    server.start(0);

    TestClient client_a(server.port());
    TestClient client_b(server.port());
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    client_a.send("this is not a valid command");
    client_a.send("click x");   // wrong arg count
    client_a.send("click a b"); // non-numeric args; stoi throws and is caught

    // client_b must keep receiving broadcasts well past the garbage sends,
    // proving the server didn't crash and didn't stall.
    REQUIRE(client_b.wait_for_messages(5, kWaitTimeout));

    client_a.close();
    client_b.close();
    server.stop();
}

}
