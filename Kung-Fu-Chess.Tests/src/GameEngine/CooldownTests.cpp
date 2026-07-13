#include "ThirdParty/doctest.h"

#include <sstream>

#include "Board.h"
#include "GameEngine.h"
#include "Parser.h"

namespace {

std::string board_of(const GameEngine& engine) {
    std::ostringstream oss;
    engine.print(oss);
    return oss.str();
}

} // namespace

TEST_SUITE("GameEngine::cooldown") {

// ---- is_selectable --------------------------------------------------------------

TEST_CASE("a piece on cooldown is not selectable") {
    Board board(1, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R, 1000 }); // cooldown ends at t=1000
    GameEngine engine(std::move(board));

    CHECK_FALSE(engine.is_selectable(Position{ 0, 0 })); // clock is still at t=0
}

TEST_CASE("a piece not on cooldown remains selectable") {
    Board board(1, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R }); // no cooldown set
    GameEngine engine(std::move(board));

    CHECK(engine.is_selectable(Position{ 0, 0 }));
}

// ---- request_move ----------------------------------------------------------------

TEST_CASE("request_move rejects a move for a piece currently on cooldown") {
    Board board(3, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R, 1000 }); // cooldown ends at t=1000
    GameEngine engine(std::move(board));

    CHECK_FALSE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }));
    CHECK(board_of(engine) == "wR . .\n"); // board unchanged
}

TEST_CASE("request_move still succeeds for a piece not on cooldown") {
    Board board(3, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R }); // no cooldown set
    GameEngine engine(std::move(board));

    CHECK(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }));
}

// ---- request_jump ----------------------------------------------------------------

TEST_CASE("request_jump rejects a jump for a piece currently on cooldown") {
    Board board(1, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R, 1000 }); // cooldown ends at t=1000
    GameEngine engine(std::move(board));

    CHECK_FALSE(engine.request_jump(Position{ 0, 0 }));
}

TEST_CASE("request_jump still succeeds for a piece not on cooldown") {
    Board board(1, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R }); // no cooldown set
    GameEngine engine(std::move(board));

    CHECK(engine.request_jump(Position{ 0, 0 }));
}

// ---- a settled move stamps its own cooldown --------------------------------------

TEST_CASE("a piece is on cooldown immediately after its move settles") {
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 })); // 2 cells of travel time
    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell); // arrives; cooldown starts now

    CHECK_FALSE(engine.is_selectable(Position{ 2, 0 }));
    CHECK_FALSE(engine.request_move(Position{ 2, 0 }, Position{ 1, 0 }));
}

TEST_CASE("a piece is selectable and movable again once its post-move cooldown elapses") {
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }));
    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell); // arrives; cooldown starts now

    engine.wait(GameEngine::kCooldownMs); // cooldown elapses
    CHECK(engine.is_selectable(Position{ 2, 0 }));
    CHECK(engine.request_move(Position{ 2, 0 }, Position{ 1, 0 }));
}

// ---- landing a jump does not stamp a cooldown (unlike a settled move) ------------

TEST_CASE("a jump-landed piece is not put on cooldown") {
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    REQUIRE(engine.request_jump(Position{ 0, 0 }));
    engine.wait(GameEngine::kJumpDurationMs); // lands

    CHECK(engine.is_selectable(Position{ 0, 0 }));
    CHECK(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }));
}

}
