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

TEST_SUITE("GameEngine::jump") {

// A piece that jumps with no enemy arriving simply lands back in place.
TEST_CASE("a jump lands on the same square when nothing arrives") {
    GameEngine engine(Parser::parse_board({
        ". . .",
        ". wK .",
        ". . .",
    }));
    engine.request_jump(Position{ 1, 1 }); // wK at (1,1) jumps for 1000 ms

    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == ". . .\n. wK .\n. . .\n");
}

// An enemy arriving during the jump window (here exactly at the landing
// instant) is captured by the airborne piece, which keeps its cell.
TEST_CASE("an airborne piece captures an enemy that arrives during the jump") {
    GameEngine engine(Parser::parse_board({
        ".  .  .",
        "wK bR .",
        ".  .  .",
    }));
    engine.request_jump(Position{ 0, 1 });                     // wK at (0,1) jumps; lands at t=1000
    engine.request_move(Position{ 1, 1 }, Position{ 0, 1 });    // bR moves onto (0,1); arrives at t=1000

    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == ". . .\nwK . .\n. . .\n");
}

// Jumping only helps while airborne: if the enemy already captured the piece,
// a later jump does nothing.
TEST_CASE("a jump after the piece was already captured does not save it") {
    GameEngine engine(Parser::parse_board({
        ".  .  .",
        "wK bR .",
        ".  .  .",
    }));
    engine.request_move(Position{ 1, 1 }, Position{ 0, 1 }); // bR moves onto (0,1), capturing wK
    engine.wait(GameEngine::kDefaultMoveMsPerCell);

    engine.request_jump(Position{ 0, 1 }); // (0,1) now holds bR; wK is gone, so this is moot
    CHECK(board_of(engine) == ". . .\nbR . .\n. . .\n");
}

// After the jump window ends, the piece is grounded again and an arriving
// enemy captures it normally.
TEST_CASE("an enemy arriving after the jump lands captures the piece normally") {
    GameEngine engine(Parser::parse_board({
        ".  . . .",
        "wK . . bR",
        ".  . . .",
    }));
    engine.request_jump(Position{ 0, 1 });                    // wK at (0,1) jumps; lands at t=1000
    engine.request_move(Position{ 3, 1 }, Position{ 0, 1 });   // bR moves onto (0,1); arrives at t=4000
    engine.wait(GameEngine::kJumpDurationMs);                 // wK back on the ground

    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == ". . . .\nbR . . .\n. . . .\n");
}

// A piece with a move already in flight cannot jump.
TEST_CASE("a moving piece cannot jump") {
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }); // wR moves to (2,0); arrives at t=2000
    engine.wait(500);                                        // still in flight

    CHECK_FALSE(engine.request_jump(Position{ 0, 0 })); // wR is moving, so the jump is rejected
    engine.wait(1500);                                  // wR completes its move
    CHECK(board_of(engine) == ". . wR\n");
}

// The airborne piece only captures enemies; a friendly piece cannot even move
// onto its (occupied) cell, so nothing changes.
TEST_CASE("an airborne piece does not affect a friendly piece") {
    GameEngine engine(Parser::parse_board({
        ".  .  .",
        "wK wR .",
        ".  .  .",
    }));
    engine.request_jump(Position{ 0, 1 }); // wK at (0,1) jumps
    CHECK_FALSE(engine.request_move(Position{ 1, 1 }, Position{ 0, 1 })); // wR onto friendly wK; rejected

    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == ". . .\nwK wR .\n. . .\n");
}

// ---- guard conditions ---------------------------------------------------------

TEST_CASE("jumping an empty cell does nothing") {
    GameEngine engine(Parser::parse_board({
        ". .",
        ". .",
    }));
    CHECK_FALSE(engine.request_jump(Position{ 1, 0 })); // (1,0) is empty
    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == ". .\n. .\n");
}

TEST_CASE("jumping is ignored once the game is over") {
    GameEngine engine(Parser::parse_board({ "wR bK" }));
    engine.request_move(Position{ 0, 0 }, Position{ 1, 0 }); // capture bK; game over
    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    REQUIRE(engine.game_over());

    CHECK_FALSE(engine.request_jump(Position{ 1, 0 })); // ignored: game is over
    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == ". wR\n");
}

// ---- an airborne piece is committed to its jump and cannot move ---------------

TEST_CASE("an airborne piece cannot be moved") {
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    engine.request_jump(Position{ 0, 0 });                                    // wR jumps; it is now committed to its cell
    CHECK_FALSE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }));     // must be rejected

    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == "wR . .\n"); // still on its original cell
}

TEST_CASE("no stale airborne record protects a cell after the window ends") {
    GameEngine engine(Parser::parse_board({
        ".  . . .",
        "wK . . bR",
        ".  . . .",
    }));
    engine.request_jump(Position{ 0, 1 });                    // wK jumps; lands at t=1000
    engine.wait(GameEngine::kJumpDurationMs);                 // wK grounded again
    engine.request_move(Position{ 3, 1 }, Position{ 0, 1 });  // bR moves onto (0,1); arrives t=4000

    engine.wait(3 * GameEngine::kDefaultMoveMsPerCell);
    // bR captures the grounded wK normally; the expired airborne record does
    // not shield the cell.
    CHECK(board_of(engine) == ". . . .\nbR . . .\n. . . .\n");
}

}
