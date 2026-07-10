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
    engine.jump(150, 150); // wK at (1,1) jumps for 1000 ms

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
    engine.jump(50, 150);   // wK at (0,1) jumps; lands at t=1000
    engine.click(150, 150); // select bR at (1,1)
    engine.click(50, 150);  // move bR onto (0,1); arrives at t=1000

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
    engine.click(150, 150); // select bR at (1,1)
    engine.click(50, 150);  // move bR onto (0,1), capturing wK
    engine.wait(GameEngine::kDefaultMoveMsPerCell);

    engine.jump(50, 150); // (0,1) now holds bR; wK is gone, so this is moot
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
    engine.jump(50, 150);       // wK at (0,1) jumps; lands at t=1000
    engine.click(350, 150);     // select bR at (3,1)
    engine.click(50, 150);      // move bR onto (0,1); arrives at t=4000
    engine.wait(GameEngine::kJumpDurationMs); // wK back on the ground

    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == ". . . .\nbR . . .\n. . . .\n");
}

// A piece with a move already in flight cannot jump.
TEST_CASE("a moving piece cannot jump") {
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    engine.click(50, 50);   // select wR at (0,0)
    engine.click(250, 50);  // move wR to (2,0); arrives at t=2000
    engine.wait(500);       // still in flight

    engine.jump(50, 50);    // wR is moving, so the jump is ignored
    engine.wait(1500);      // wR completes its move
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
    engine.jump(50, 150);   // wK at (0,1) jumps
    engine.click(150, 150); // select wR at (1,1)
    engine.click(50, 150);  // try to move wR onto friendly wK; rejected

    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == ". . .\nwK wR .\n. . .\n");
}

// ---- guard conditions ---------------------------------------------------------

TEST_CASE("jumping an empty cell does nothing") {
    GameEngine engine(Parser::parse_board({
        ". .",
        ". .",
    }));
    engine.jump(150, 50); // (1,0) is empty
    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == ". .\n. .\n");
}

TEST_CASE("jumping is ignored once the game is over") {
    GameEngine engine(Parser::parse_board({ "wR bK" }));
    engine.click(50, 50);  // select wR at (0,0)
    engine.click(150, 50); // capture bK; game over
    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    REQUIRE(engine.game_over());

    engine.jump(150, 50);  // ignored: game is over
    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == ". wR\n");
}

// ---- an airborne piece is committed to its jump and cannot move ---------------

TEST_CASE("an airborne piece cannot be moved") {
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    engine.click(50, 50);  // select wR at (0,0)
    engine.jump(50, 50);   // wR jumps; it is now committed to its cell
    engine.click(250, 50); // try to move it to (2,0); must be rejected

    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == "wR . .\n"); // still on its original cell
}

TEST_CASE("selecting then jumping the same piece clears the selection") {
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    engine.click(50, 50); // select wR at (0,0)
    REQUIRE(engine.has_selection());

    engine.jump(50, 50);  // jumping the selected piece drops the selection
    CHECK_FALSE(engine.has_selection());

    engine.click(250, 50); // nothing selected, empty cell -> no move scheduled
    engine.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(engine) == "wR . .\n");
}

TEST_CASE("no stale airborne record protects a cell after the window ends") {
    GameEngine engine(Parser::parse_board({
        ".  . . .",
        "wK . . bR",
        ".  . . .",
    }));
    engine.jump(50, 150);                     // wK jumps; lands at t=1000
    engine.wait(GameEngine::kJumpDurationMs); // wK grounded again
    engine.click(350, 150);                   // select bR at (3,1)
    engine.click(50, 150);                    // move bR onto (0,1); arrives t=4000

    engine.wait(3 * GameEngine::kDefaultMoveMsPerCell);
    // bR captures the grounded wK normally; the expired airborne record does
    // not shield the cell.
    CHECK(board_of(engine) == ". . . .\nbR . . .\n. . . .\n");
}

}
