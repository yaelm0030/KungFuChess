#include "ThirdParty/doctest.h"

#include <chrono>

#include "Parser.h"
#include "ShardAndGateway.h"
#include "../GameServer/TestClient.h"
#include "../UserRepository/FakeUserRepository.h"
#include "../UserRepository/ThrowingUserRepository.h"

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

TEST_SUITE("WebSocketGateway::username") {

TEST_CASE("before any name message, a connection's username is unset") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    CHECK(sg.gateway.username(Color::w) == std::nullopt);

    client.close();
}

TEST_CASE("a name message sets the username for that connection's color") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name Alice");
    REQUIRE(client.wait_for_messages(2, kWaitTimeout));

    CHECK(sg.gateway.username(Color::w) == "Alice");

    client.close();
}

TEST_CASE("two connections track independent usernames") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client_a(sg.gateway.port()); // White
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    TestClient client_b(sg.gateway.port()); // Black
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    client_a.send("name Alice");
    REQUIRE(client_a.wait_for_messages(2, kWaitTimeout));
    client_b.send("name Bob");
    REQUIRE(client_b.wait_for_messages(2, kWaitTimeout));

    CHECK(sg.gateway.username(Color::w) == "Alice");
    CHECK(sg.gateway.username(Color::b) == "Bob");

    client_a.close();
    client_b.close();
}

TEST_CASE("a second name message overwrites the first") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name Alice");
    REQUIRE(client.wait_for_messages(2, kWaitTimeout));
    client.send("name Alicia");
    REQUIRE(client.wait_for_messages(3, kWaitTimeout));

    CHECK(sg.gateway.username(Color::w) == "Alicia");

    client.close();
}

TEST_CASE("a multi-word name message is stored joined with single spaces") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name John Smith");
    REQUIRE(client.wait_for_messages(2, kWaitTimeout));

    CHECK(sg.gateway.username(Color::w) == "John Smith");

    client.close();
}

TEST_CASE("a spectator's name message doesn't affect White/Black state and doesn't crash the gateway") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client_a(sg.gateway.port()); // White
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    TestClient client_b(sg.gateway.port()); // Black
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    TestClient client_c(sg.gateway.port()); // spectator
    REQUIRE(client_c.wait_for_messages(1, kWaitTimeout));

    client_c.send("name Carol");

    // client_c must keep receiving broadcasts well past its name message,
    // proving the gateway didn't crash and didn't stall.
    REQUIRE(client_c.wait_for_messages(6, kWaitTimeout));
    CHECK(sg.gateway.username(Color::w) == std::nullopt);
    CHECK(sg.gateway.username(Color::b) == std::nullopt);

    client_a.close();
    client_b.close();
    client_c.close();
}

TEST_CASE("a malformed name message with no username token is ignored") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name");

    // No reply is expected for "name" alone; use a later broadcast tick as a
    // clock to give the gateway time to (not) process it.
    REQUIRE(client.wait_for_messages(3, kWaitTimeout));
    CHECK(sg.gateway.username(Color::w) == std::nullopt);

    client.close();
}

TEST_CASE("a disconnect clears the username along with the color slot") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client_a(sg.gateway.port()); // White
    REQUIRE(client_a.wait_for_messages(1, kWaitTimeout));
    TestClient client_b(sg.gateway.port()); // Black
    REQUIRE(client_b.wait_for_messages(1, kWaitTimeout));

    client_a.send("name Alice");
    REQUIRE(client_a.wait_for_messages(2, kWaitTimeout));
    client_a.close();

    // Give the gateway's close handler time to free the White slot before the
    // next connection arrives, using client_b's continued broadcasts as a clock.
    REQUIRE(client_b.wait_for_messages(8, kWaitTimeout));

    TestClient client_c(sg.gateway.port()); // becomes the new White
    REQUIRE(client_c.wait_for_messages(1, kWaitTimeout));

    CHECK(sg.gateway.username(Color::w) == std::nullopt);

    client_b.close();
    client_c.close();
}

}

TEST_SUITE("WebSocketGateway::rating") {

TEST_CASE("a name message for a never-seen username stores the default rating") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name Alice");
    REQUIRE(client.wait_for_messages(2, kWaitTimeout));

    CHECK(sg.gateway.rating(Color::w) == kDefaultUserRating);

    client.close();
}

TEST_CASE("a name message for an already-known username returns its existing rating") {
    FakeUserRepository repository({ { "Alice", 1500 } });
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name Alice");
    REQUIRE(client.wait_for_messages(2, kWaitTimeout));

    CHECK(sg.gateway.rating(Color::w) == 1500);

    client.close();
}

TEST_CASE("a second name message re-resolves the rating for the new username") {
    FakeUserRepository repository({ { "Alice", 1500 }, { "Bob", 1200 } });
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name Alice");
    REQUIRE(client.wait_for_messages(2, kWaitTimeout));
    client.send("name Bob");
    REQUIRE(client.wait_for_messages(3, kWaitTimeout));

    CHECK(sg.gateway.rating(Color::w) == 1200);

    client.close();
}

TEST_CASE("before any name message, a connection's rating is unset") {
    FakeUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    CHECK(sg.gateway.rating(Color::w) == std::nullopt);

    client.close();
}

TEST_CASE("a repository failure keeps the username but leaves the rating unset, without disrupting the connection") {
    ThrowingUserRepository repository;
    ShardAndGateway sg(make_board(), repository);
    sg.gateway.start(0);

    TestClient client(sg.gateway.port()); // becomes White
    REQUIRE(client.wait_for_messages(1, kWaitTimeout));

    client.send("name Alice");
    REQUIRE(client.wait_for_messages(2, kWaitTimeout));

    CHECK(sg.gateway.username(Color::w) == "Alice");
    CHECK(sg.gateway.rating(Color::w) == std::nullopt);

    // The connection must keep receiving broadcasts well past the throw,
    // proving the gateway didn't crash and didn't stall.
    REQUIRE(client.wait_for_messages(6, kWaitTimeout));

    client.close();
}

}
