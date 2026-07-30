#include "ThirdParty/doctest.h"

#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <thread>

#include "Constants.h"
#include "GameEngine.h"
#include "GameServer.h"
#include "Parser.h"
#include "Position.h"
#include "../UserRepository/FakeUserRepository.h"

namespace {

Board make_board() {
    return Parser::parse_board({
        "bR bN .",
        ".  .  .",
        "wR .  wN",
    });
}

std::optional<PieceSnapshot> find_piece(const GameSnapshot& snap, Color color, PieceType type) {
    for (const PieceSnapshot& piece : snap.pieces) {
        if (piece.color == color && piece.type == type) {
            return piece;
        }
    }
    return std::nullopt;
}

// A regression here is "stop() hangs forever", so stop() runs on a helper thread and
// the test waits on its future with a timeout instead of joining it directly. On
// timeout the helper thread is detached rather than joined, so server/stopped are
// captured by shared_ptr (not by reference) to stay alive for it: a reference into the
// test's stack frame would dangle the moment the TEST_CASE returns and its locals are
// destructed, racing the still-running detached thread. A confirmed regression leaks
// the server deliberately instead of risking that use-after-free.
bool stop_completes_within(std::shared_ptr<GameServer> server, std::chrono::milliseconds timeout) {
    auto stopped = std::make_shared<std::promise<void>>();
    std::future<void> stopped_future = stopped->get_future();
    std::thread stopper([server, stopped]() {
        server->stop();
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

TEST_SUITE("GameServer::start/stop") {

TEST_CASE("start then stop returns promptly without hanging or crashing") {
    FakeUserRepository repository;
    GameServer server(make_board(), repository);

    server.start(0);
    server.stop();

    CHECK_FALSE(server.snapshot().is_game_over);
}

TEST_CASE("a command applied while the tick thread is running is eventually reflected in snapshot") {
    FakeUserRepository repository;
    GameServer server(make_board(), repository);

    server.start(0);
    server.apply_command("click 50 50"); // select bR at (0,0)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.stop();

    CHECK(server.snapshot().selected == Position{ 0, 0 });
}

TEST_CASE("the tick thread advances the clock on its own with no external tick call") {
    const long long move_ms_per_cell = 100; // small, so the test proves this in tens of ms, not over a second
    FakeUserRepository repository;
    GameServer server(make_board(), repository, move_ms_per_cell);

    server.start(0);
    server.apply_command("click 50 50");  // select bR at (0,0)
    server.apply_command("click 50 150"); // move down to (0,1); 1 cell of travel time
    std::this_thread::sleep_for(std::chrono::milliseconds(move_ms_per_cell + 100));
    server.stop();

    std::optional<PieceSnapshot> br = find_piece(server.snapshot(), Color::b, PieceType::R);
    REQUIRE(br.has_value());
    CHECK(br->pixels_location.x == 0);
    CHECK(br->pixels_location.y == constants::kCellSizePx);
}

TEST_CASE("stop is idempotent when called twice in a row") {
    FakeUserRepository repository;
    GameServer server(make_board(), repository);

    server.start(0);
    server.stop();
    server.stop();

    CHECK_FALSE(server.snapshot().is_game_over);
}

TEST_CASE("a server that goes out of scope while still running tears down cleanly") {
    {
        FakeUserRepository repository;
        GameServer server(make_board(), repository);
        server.start(0);
    }

    CHECK(true);
}

TEST_CASE("stop returns within a bounded time after the io thread has gone idle") {
    FakeUserRepository repository;
    auto server = std::make_shared<GameServer>(make_board(), repository);

    server->start(0);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    CHECK(stop_completes_within(server, std::chrono::seconds(5)));
}

TEST_CASE("stop returns within a bounded time on repeated idle-then-stop cycles") {
    for (int i = 0; i < 10; ++i) {
        FakeUserRepository repository;
        auto server = std::make_shared<GameServer>(make_board(), repository);

        server->start(0);
        std::this_thread::sleep_for(std::chrono::seconds(2));

        CHECK(stop_completes_within(server, std::chrono::seconds(5)));
    }
}

}
