#include "ThirdParty/doctest.h"

#include <sstream>

#include "GameEngine.h"
#include "Parser.h"

namespace {

Board make_board() {
    return Parser::parse_board({
        "bR bN .",
        ".  .  .",
        "wR .  wN",
    });
}

std::string board_of(const GameEngine& engine) {
    std::ostringstream oss;
    engine.print(oss);
    return oss.str();
}

} // namespace

TEST_SUITE("GameEngine::click") {

// ---- outside the board ---------------------------------------------------

TEST_CASE("clicking outside the board is ignored") {
    SUBCASE("negative x") {
        GameEngine engine(make_board());
        engine.click(-50, 50);
        CHECK_FALSE(engine.has_selection());
    }
    SUBCASE("negative y") {
        GameEngine engine(make_board());
        engine.click(50, -50);
        CHECK_FALSE(engine.has_selection());
    }
    SUBCASE("x past the last column") {
        GameEngine engine(make_board());
        engine.click(350, 50); // board is 3 columns wide (0-299px)
        CHECK_FALSE(engine.has_selection());
    }
    SUBCASE("y past the last row") {
        GameEngine engine(make_board());
        engine.click(50, 350); // board is 3 rows tall (0-299px)
        CHECK_FALSE(engine.has_selection());
    }
}

TEST_CASE("clicking on a degenerate 0x0 board is always ignored") {
    GameEngine engine(Parser::parse_board({}));
    engine.click(0, 0);
    engine.click(50, 50);
    CHECK_FALSE(engine.has_selection());
}

// ---- pixel-to-cell mapping -------------------------------------------------

TEST_CASE("pixel coordinates map to the containing cell, not just its center") {
    GameEngine engine(make_board());

    SUBCASE("top-left pixel of a cell") {
        engine.click(0, 0);
        REQUIRE(engine.has_selection());
        CHECK(engine.selected()->x == 0);
        CHECK(engine.selected()->y == 0);
    }
    SUBCASE("bottom-right pixel of the same cell (99,99) stays in cell (0,0)") {
        engine.click(99, 99);
        REQUIRE(engine.has_selection());
        CHECK(engine.selected()->x == 0);
        CHECK(engine.selected()->y == 0);
    }
    SUBCASE("crossing the boundary at x=100 selects the piece in the next cell") {
        engine.click(100, 50); // cell (1,0) = bN
        REQUIRE(engine.has_selection());
        CHECK(engine.selected()->x == 1);
        CHECK(engine.selected()->y == 0);
    }
    SUBCASE("center of a piece cell, per the spec example") {
        engine.click(50, 50);
        REQUIRE(engine.has_selection());
        CHECK(engine.selected()->x == 0);
        CHECK(engine.selected()->y == 0);
    }
}

// ---- selecting with nothing currently selected -----------------------------

TEST_CASE("clicking an empty cell with no selection is ignored") {
    GameEngine engine(make_board());
    engine.click(250, 50);  // (2,0) is empty
    engine.click(150, 150); // (1,1) is empty
    CHECK_FALSE(engine.has_selection());
}

TEST_CASE("clicking a piece with no selection selects it") {
    GameEngine engine(make_board());
    engine.click(50, 50); // (0,0) = bR
    REQUIRE(engine.has_selection());
    CHECK(engine.selected()->x == 0);
    CHECK(engine.selected()->y == 0);
}

// ---- replacing the selection ----------------------------------------------

TEST_CASE("clicking another friendly piece replaces the selection") {
    GameEngine engine(make_board());
    engine.click(50, 50);   // select bR at (0,0)
    engine.click(150, 50);  // bN at (1,0) is also black
    REQUIRE(engine.has_selection());
    CHECK(engine.selected()->x == 1);
    CHECK(engine.selected()->y == 0);
    CHECK(board_of(engine) == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("re-clicking the same selected piece keeps it selected") {
    GameEngine engine(make_board());
    engine.click(50, 50);
    engine.click(50, 50);
    REQUIRE(engine.has_selection());
    CHECK(engine.selected()->x == 0);
    CHECK(engine.selected()->y == 0);
}

// ---- move requests (settle after the travel time elapses) ------------------

TEST_CASE("clicking an empty cell while a piece is selected eventually moves it there") {
    GameEngine engine(make_board());
    engine.click(50, 50);  // select bR at (0,0)
    engine.click(50, 150); // (0,1) is empty; straight down is a legal rook move

    CHECK_FALSE(engine.has_selection());
    engine.wait(GameEngine::kDefaultMoveMsPerCell); // 1 cell of travel time
    CHECK(board_of(engine) == ". bN .\nbR . .\nwR . wN\n");
}

TEST_CASE("clicking an enemy piece while a piece is selected eventually captures it") {
    GameEngine engine(make_board());
    engine.click(50, 50);   // select bR at (0,0)
    engine.click(50, 250);  // wR at (0,2) is white; straight down the column

    CHECK_FALSE(engine.has_selection());
    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell); // 2 cells of travel time
    CHECK(board_of(engine) == ". bN .\n. . .\nbR . wN\n");
}

TEST_CASE("the vacated source cell is empty once the move has arrived") {
    GameEngine engine(make_board());
    engine.click(50, 50);  // select bR at (0,0)
    engine.click(50, 150); // move down to (0,1)
    engine.wait(GameEngine::kDefaultMoveMsPerCell);

    engine.click(50, 50); // (0,0) is now empty; no selection to move
    CHECK_FALSE(engine.has_selection());
}

TEST_CASE("a cell with a move in flight cannot be reselected") {
    GameEngine engine(make_board());
    engine.click(50, 50);  // select bR at (0,0)
    engine.click(50, 150); // move down to (0,1); still in flight

    engine.click(50, 50); // (0,0) still shows bR, but it's mid-move
    CHECK_FALSE(engine.has_selection());
}

TEST_CASE("consecutive moves chain correctly once each one arrives") {
    GameEngine engine(make_board());
    engine.click(50, 50);  // select bR at (0,0)
    engine.click(50, 150); // move bR down to (0,1)
    engine.wait(GameEngine::kDefaultMoveMsPerCell);

    engine.click(50, 150); // re-select the piece that is now at (0,1)
    REQUIRE(engine.has_selection());
    CHECK(engine.selected()->x == 0);
    CHECK(engine.selected()->y == 1);

    engine.click(50, 250); // move down to (0,2), capturing wR
    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK_FALSE(engine.has_selection());
    CHECK(board_of(engine) == ". bN .\n. . .\nbR . wN\n");
}

// ---- movement over time -----------------------------------------------------

TEST_CASE("the board still shows the piece at its original cell before arrival") {
    GameEngine engine(make_board());
    engine.click(50, 50);  // select bR at (0,0)
    engine.click(50, 150); // 1 cell of travel time is needed to arrive

    engine.wait(GameEngine::kDefaultMoveMsPerCell - 1); // one millisecond short
    CHECK(board_of(engine) == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("the piece appears at the destination once enough time has passed") {
    GameEngine engine(make_board());
    engine.click(50, 50);  // select bR at (0,0)
    engine.click(50, 150); // 1 cell of travel time is needed to arrive

    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == ". bN .\nbR . .\nwR . wN\n");
}

// ---- a moving piece cannot be redirected; no cooldown once it arrives ------

TEST_CASE("a piece already moving cannot be redirected to a new destination") {
    GameEngine engine(make_board());
    engine.click(50, 50);  // select bR at (0,0)
    engine.click(50, 150); // move down to (0,1); 1 cell of travel time

    // Attempt to redirect it mid-route by reselecting its (still visible)
    // origin cell; this fails, so there is no selection left to redirect.
    engine.click(50, 50);
    CHECK_FALSE(engine.has_selection());

    // Once the original move arrives, the piece is at its first destination
    // only; the redirect attempt had no effect.
    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == ". bN .\nbR . .\nwR . wN\n");
}

TEST_CASE("a piece can be selected and moved again immediately after arriving, with no cooldown") {
    GameEngine engine(make_board());
    engine.click(50, 50);  // select bR at (0,0)
    engine.click(50, 150); // move down to (0,1)
    engine.wait(GameEngine::kDefaultMoveMsPerCell); // arrives; no extra wait afterward

    engine.click(50, 150); // select the just-arrived piece right away
    REQUIRE(engine.has_selection());
    CHECK(engine.selected()->x == 0);
    CHECK(engine.selected()->y == 1);

    engine.click(50, 250); // immediately move it again, down to (0,2), capturing wR
    CHECK_FALSE(engine.has_selection());
    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == ". bN .\n. . .\nbR . wN\n");
}

// ---- moves that don't match the piece's shape are ignored -------------------

TEST_CASE("a move that doesn't match the selected piece's shape is ignored") {
    GameEngine engine(make_board());
    engine.click(50, 50);   // select bR at (0,0)
    engine.click(150, 150); // (1,1) is a diagonal move; illegal for a rook

    REQUIRE(engine.has_selection());
    CHECK(engine.selected()->x == 0);
    CHECK(engine.selected()->y == 0);
    CHECK(board_of(engine) == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("a blocked straight move is ignored") {
    GameEngine engine(Parser::parse_board({
        "bR .",
        "bN .",
        ".  .",
    }));
    engine.click(50, 50);  // select bR at (0,0)
    engine.click(50, 250); // (0,2) is past bN, which blocks the column at (0,1)

    REQUIRE(engine.has_selection());
    CHECK(engine.selected()->x == 0);
    CHECK(engine.selected()->y == 0);
}

// ---- colliding moves on a shared route --------------------------------------
//relate to conflicts_with_pending_move commented function in GameEngine.cpp

//TEST_CASE("when two pieces attempt to swap places along the same route, whichever moved first wins") {
//    SUBCASE("white moves first") {
//        GameEngine engine(Parser::parse_board({ "wR . . bR" }));
//        engine.click(50, 50);   // select wR at (0,0)
//        engine.click(350, 50);  // move wR across to (3,0), capturing bR; 3 cells of travel time
//        engine.click(350, 50);  // select bR at (3,0); it hasn't moved yet
//        engine.click(50, 50);   // attempt to move bR back to (0,0); collides with wR's route
//
//        engine.wait(3 * GameEngine::kDefaultMoveMsPerCell);
//        CHECK(board_of(engine) == ". . . wR\n");
//    }
//
//    SUBCASE("black moves first") {
//        GameEngine engine(Parser::parse_board({ "wR . . bR" }));
//        engine.click(350, 50);  // select bR at (3,0)
//        engine.click(50, 50);   // move bR across to (0,0), capturing wR; 3 cells of travel time
//        engine.click(50, 50);   // select wR at (0,0); it hasn't moved yet
//        engine.click(350, 50);  // attempt to move wR back to (3,0); collides with bR's route
//
//        engine.wait(3 * GameEngine::kDefaultMoveMsPerCell);
//        CHECK(board_of(engine) == "bR . . .\n");
//    }
//}

//TEST_CASE("a move rejected for colliding with another move's route keeps the current selection") {
//    GameEngine engine(Parser::parse_board({ "wR . . bR" }));
//    engine.click(50, 50);  // select wR at (0,0)
//    engine.click(350, 50); // move wR across to (3,0); still in flight
//    engine.click(350, 50); // select bR at (3,0)
//    engine.click(50, 50);  // attempt to move bR back to (0,0); collides, rejected
//
//    REQUIRE(engine.has_selection());
//    CHECK(engine.selected()->x == 3);
//    CHECK(engine.selected()->y == 0);
//}
//
//TEST_CASE("a move onto another route's cell is rejected for its own illegality, not treated as a collision") {
//    GameEngine engine(Parser::parse_board({
//        ".  .  .  .",
//        "wQ .  .  bK",
//        ".  .  bP .",
//        ".  .  .  .",
//    }));
//    engine.click(50, 150);  // select wQ at (0,1)
//    engine.click(350, 150); // move wQ across to (3,1), capturing bK; 3 cells of travel time
//    engine.wait(200);
//
//    engine.click(250, 250); // select bP at (2,2)
//    engine.click(250, 150); // (2,1) is on wQ's route, but this move is illegal anyway: backward for black
//
//    engine.wait(3000);
//    CHECK(board_of(engine) == ". . . .\n. . . wQ\n. . bP .\n. . . .\n");
//}

// ---- friendly-piece landing ---------------------------------------------------

TEST_CASE("a knight cannot land on a friendly piece, even though the move shape is legal") {
    GameEngine engine(Parser::parse_board({
        ".  wP .",
        ".  .  .",
        "wN .  .",
    }));
    engine.click(50, 250); // select wN at (0,2)
    engine.click(150, 50); // attempt an L-shaped move onto wP at (1,0); same color

    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == ". wP .\n. . .\nwN . .\n");
}

// ---- premoves that never became legal ------------------------------------------

TEST_CASE("a click on an unrelated empty cell after a failed redirect schedules nothing") {
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    engine.click(50, 50);  // select wR at (0,0)
    engine.click(150, 50); // move to (1,0); 1 cell of travel time
    engine.click(50, 50);  // attempt to redirect; fails, nothing selected
    engine.click(250, 50); // (2,0) is empty and nothing is selected; ignored

    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == ". wR .\n");
}

}
