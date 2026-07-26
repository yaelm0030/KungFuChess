#include "ThirdParty/doctest.h"

#include <chrono>

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

TEST_SUITE("GameServer::username") {

TEST_CASE("before any name message, a connection's username is unset") {
    GameServer server(make_board());
    server.start(0);

    TestClient client(server.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    CHECK(server.username(Color::w) == std::nullopt);

    client.close();
    server.stop();
}

TEST_CASE("a name message sets the username for that connection's color") {
    GameServer server(make_board());
    server.start(0);

    TestClient client(server.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name Alice");
    REQUIRE(client.wait_for_messages(2, kWaitTimeout));

    CHECK(server.username(Color::w) == "Alice");

    client.close();
    server.stop();
}

TEST_CASE("two connections track independent usernames") {
    GameServer server(make_board());
    server.start(0);

    TestClient client_a(server.port()); // White
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    TestClient client_b(server.port()); // Black
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    client_a.send("name Alice");
    REQUIRE(client_a.wait_for_messages(2, kWaitTimeout));
    client_b.send("name Bob");
    REQUIRE(client_b.wait_for_messages(2, kWaitTimeout));

    CHECK(server.username(Color::w) == "Alice");
    CHECK(server.username(Color::b) == "Bob");

    client_a.close();
    client_b.close();
    server.stop();
}

TEST_CASE("a second name message overwrites the first") {
    GameServer server(make_board());
    server.start(0);

    TestClient client(server.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name Alice");
    REQUIRE(client.wait_for_messages(2, kWaitTimeout));
    client.send("name Alicia");
    REQUIRE(client.wait_for_messages(3, kWaitTimeout));

    CHECK(server.username(Color::w) == "Alicia");

    client.close();
    server.stop();
}

TEST_CASE("a spectator's name message doesn't affect White/Black state and doesn't crash the server") {
    GameServer server(make_board());
    server.start(0);

    TestClient client_a(server.port()); // White
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    TestClient client_b(server.port()); // Black
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    TestClient client_c(server.port()); // spectator
    REQUIRE(client_c.wait_for_messages(1, kWaitTimeout));

    client_c.send("name Carol");

    // client_c must keep receiving broadcasts well past its name message,
    // proving the server didn't crash and didn't stall.
    REQUIRE(client_c.wait_for_messages(6, kWaitTimeout));
    CHECK(server.username(Color::w) == std::nullopt);
    CHECK(server.username(Color::b) == std::nullopt);

    client_a.close();
    client_b.close();
    client_c.close();
    server.stop();
}

TEST_CASE("a malformed name message with no username token is ignored") {
    GameServer server(make_board());
    server.start(0);

    TestClient client(server.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name");

    // No reply is expected for "name" alone; use a later broadcast tick as a
    // clock to give the server time to (not) process it.
    REQUIRE(client.wait_for_messages(3, kWaitTimeout));
    CHECK(server.username(Color::w) == std::nullopt);

    client.close();
    server.stop();
}

TEST_CASE("a disconnect clears the username along with the color slot") {
    GameServer server(make_board());
    server.start(0);

    TestClient client_a(server.port()); // White
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    TestClient client_b(server.port()); // Black
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    client_a.send("name Alice");
    REQUIRE(client_a.wait_for_messages(2, kWaitTimeout));
    client_a.close();

    // Give the server's close handler time to free the White slot before the
    // next connection arrives, using client_b's continued broadcasts as a clock.
    REQUIRE(client_b.wait_for_messages(8, kWaitTimeout));

    TestClient client_c(server.port()); // becomes the new White
    REQUIRE(client_c.wait_for_messages(1, kWaitTimeout));

    CHECK(server.username(Color::w) == std::nullopt);

    client_b.close();
    client_c.close();
    server.stop();
}

}
