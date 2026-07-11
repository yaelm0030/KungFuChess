#include "ThirdParty/doctest.h"

#include <sstream>

#include "GameEngine.h"
#include "Parser.h"

namespace {

std::string board_of(const GameEngine& engine) {
    std::ostringstream oss;
    engine.print(oss);
    return oss.str();
}

} // namespace

TEST_SUITE("GameEngine::game_over") {

TEST_CASE("the game is not over before any king is captured") {
    GameEngine engine(Parser::parse_board({ "wR . bK" }));
    CHECK_FALSE(engine.game_over());
}

TEST_CASE("capturing the enemy king ends the game once the move settles") {
    GameEngine engine(Parser::parse_board({ "wR . bK" }));
    engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }); // wR captures bK; 2 cells of travel time

    CHECK_FALSE(engine.game_over()); // not settled yet
    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK(engine.game_over());
    CHECK(board_of(engine) == ". . wR\n");
}

TEST_CASE("capturing a non-king piece does not end the game") {
    GameEngine engine(Parser::parse_board({ "wR . bR" }));
    engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }); // wR captures bR

    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK_FALSE(engine.game_over());
}

TEST_CASE("once the game is over, further move requests are ignored") {
    GameEngine engine(Parser::parse_board({ "wR . bK", "wN . ." }));
    engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }); // wR captures bK
    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    REQUIRE(engine.game_over());

    CHECK_FALSE(engine.request_move(Position{ 0, 1 }, Position{ 0, 0 })); // wN attempts to move; ignored
    CHECK(board_of(engine) == ". . wR\nwN . .\n");
}

TEST_CASE("a move already in flight when the game ends still settles on the board") {
    GameEngine engine(Parser::parse_board({
        "wR .  bK .",
        ".  .  .  .",
        "wN .  .  .",
        ".  .  .  .",
    }));
    engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }); // wR captures bK; 2 cells of travel time
    engine.request_move(Position{ 0, 2 }, Position{ 2, 3 }); // wN's L-shaped move, scheduled before the king capture settles; also 2 cells of travel time

    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK(engine.game_over());
    CHECK(board_of(engine) == ". . wR .\n. . . .\n. . . .\n. . wN .\n");
}

// Characterization test (pins existing behavior before any refactor): an
// enemy piece can jump onto its own cell to guard it, and if a king arrives
// on that cell while the guard is still airborne, the guard captures the
// arriving king instead of being captured itself. The king is destroyed
// (removed from its origin) and the game ends; the guard is left untouched,
// exactly as if it never moved.
TEST_CASE("a king arriving on a cell guarded by a still-airborne enemy piece is destroyed and ends the game") {
    GameEngine engine(Parser::parse_board({ "wK bR" }));
    engine.request_jump(Position{ 1, 0 }); // bR at (1,0) jumps, guarding its cell for kJumpDurationMs
    engine.request_move(Position{ 0, 0 }, Position{ 1, 0 }); // wK moves onto bR's guarded cell; 1 cell of travel time

    CHECK_FALSE(engine.game_over()); // not settled yet

    engine.wait(GameEngine::kDefaultMoveMsPerCell); // arrives exactly as the guard lands
    CHECK(engine.game_over());
    CHECK(board_of(engine) == ". bR\n"); // king destroyed; guard untouched on its cell
}

// Characterization test: a normal capture-on-arrival and an airborne-guard
// capture resolving within the same wait() call are independent of each
// other — settling one does not skip or short-circuit the other.
TEST_CASE("a normal capture and an airborne-guard capture settle independently within the same wait") {
    GameEngine engine(Parser::parse_board({ "wR bN . wQ bQ" }));
    engine.request_jump(Position{ 4, 0 }); // bQ at (4,0) jumps, guarding its cell

    engine.request_move(Position{ 0, 0 }, Position{ 1, 0 }); // wR onto bN at (1,0); normal capture-on-arrival
    engine.request_move(Position{ 3, 0 }, Position{ 4, 0 }); // wQ onto bQ's guarded cell; destroyed by the guard on arrival

    engine.wait(GameEngine::kDefaultMoveMsPerCell); // both moves arrive together
    CHECK_FALSE(engine.game_over()); // no king involved in either skirmish
    CHECK(board_of(engine) == ". wR . . bQ\n"); // bN captured normally; wQ destroyed, guard untouched
}

}
