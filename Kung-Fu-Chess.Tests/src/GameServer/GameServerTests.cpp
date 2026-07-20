#include "ThirdParty/doctest.h"

#include <optional>

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

TEST_SUITE("GameServer::tick") {

TEST_CASE("tick advances the clock like Controller::wait, arriving after the full travel time") {
    GameServer server(make_board());
    server.apply_command("click 50 50");  // select bR at (0,0)
    server.apply_command("click 50 150"); // move down to (0,1); 1 cell of travel time

    server.tick(static_cast<int>(GameEngine::kDefaultMoveMsPerCell));

    std::optional<PieceSnapshot> br = find_piece(server.snapshot(), Color::b, PieceType::R);
    REQUIRE(br.has_value());
    CHECK(br->pixels_location.x == 0);
    CHECK(br->pixels_location.y == constants::kCellSizePx);
}

TEST_CASE("tick one millisecond short of the travel time leaves the move mid-flight") {
    GameServer server(make_board());
    server.apply_command("click 50 50");  // select bR at (0,0)
    server.apply_command("click 50 150"); // 1 cell of travel time is needed to arrive

    server.tick(static_cast<int>(GameEngine::kDefaultMoveMsPerCell) - 1);

    std::optional<PieceSnapshot> br = find_piece(server.snapshot(), Color::b, PieceType::R);
    REQUIRE(br.has_value());
    CHECK(br->state == PieceState::move);
}

}

TEST_SUITE("GameServer::apply_command") {

TEST_CASE("a well-formed click line selects the piece at that cell") {
    GameServer server(make_board());

    server.apply_command("click 50 50"); // (0,0) = bR

    CHECK(server.snapshot().selected == Position{ 0, 0 });
}

TEST_CASE("a well-formed jump line on an already-selected piece clears the selection") {
    GameServer server(make_board());
    server.apply_command("click 50 50"); // select bR at (0,0)
    REQUIRE(server.snapshot().selected == Position{ 0, 0 });

    server.apply_command("jump 50 50");

    CHECK(server.snapshot().selected == std::nullopt);
}

TEST_CASE("a wait command sent through apply_command is ignored; GameServer stays the sole clock authority") {
    GameServer server(make_board());
    server.apply_command("click 50 50");  // select bR at (0,0)
    server.apply_command("click 50 150"); // schedule move to (0,1); still in flight

    server.apply_command("wait 1000");

    std::optional<PieceSnapshot> br = find_piece(server.snapshot(), Color::b, PieceType::R);
    REQUIRE(br.has_value());
    CHECK(br->pixels_location.x == 0); // still at the origin cell (0,0)
    CHECK(br->pixels_location.y == 0);
    CHECK(br->state == PieceState::move); // still in flight; a real wait(1000) would have settled it
}

TEST_CASE("a print command is ignored") {
    GameServer server(make_board());

    server.apply_command("print board");

    CHECK(server.snapshot().selected == std::nullopt);
    std::optional<PieceSnapshot> br = find_piece(server.snapshot(), Color::b, PieceType::R);
    REQUIRE(br.has_value());
    CHECK(br->pixels_location.x == 0);
    CHECK(br->pixels_location.y == 0);
    CHECK(br->state == PieceState::idle);
}

TEST_CASE("malformed or empty input is ignored without throwing") {
    GameServer server(make_board());

    SUBCASE("wrong arg count") {
        server.apply_command("click 5");
    }
    SUBCASE("non-numeric args") {
        server.apply_command("click a b");
    }
    SUBCASE("unknown command") {
        server.apply_command("foo 1 2");
    }
    SUBCASE("blank line") {
        server.apply_command("");
    }

    CHECK(server.snapshot().selected == std::nullopt);
}

}

TEST_SUITE("GameServer::snapshot") {

TEST_CASE("a freshly constructed server's snapshot reflects the board it was constructed with") {
    GameServer server(make_board());

    GameSnapshot snap = server.snapshot();

    CHECK(snap.board_width == 3);
    CHECK(snap.board_height == 3);
    CHECK(snap.pieces.size() == 4);
    CHECK_FALSE(snap.is_game_over);
}

}
