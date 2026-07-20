#include "ThirdParty/doctest.h"

#include <chrono>
#include <optional>
#include <thread>

#include "Constants.h"
#include "GameEngine.h"
#include "GameServer.h"
#include "Parser.h"
#include "Position.h"

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

} // namespace

TEST_SUITE("GameServer::start/stop") {

TEST_CASE("start then stop returns promptly without hanging or crashing") {
    GameServer server(make_board());

    server.start();
    server.stop();

    CHECK_FALSE(server.snapshot().is_game_over);
}

TEST_CASE("a command applied while the tick thread is running is eventually reflected in snapshot") {
    GameServer server(make_board());

    server.start();
    server.apply_command("click 50 50"); // select bR at (0,0)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.stop();

    CHECK(server.snapshot().selected == Position{ 0, 0 });
}

TEST_CASE("the tick thread advances the clock on its own with no external tick call") {
    const long long move_ms_per_cell = 100; // small, so the test proves this in tens of ms, not over a second
    GameServer server(make_board(), move_ms_per_cell);

    server.start();
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
    GameServer server(make_board());

    server.start();
    server.stop();
    server.stop();

    CHECK_FALSE(server.snapshot().is_game_over);
}

TEST_CASE("a server that goes out of scope while still running tears down cleanly") {
    {
        GameServer server(make_board());
        server.start();
    }

    CHECK(true);
}

}
