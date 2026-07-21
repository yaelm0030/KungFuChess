#include "ThirdParty/doctest.h"

#include <sstream>

#include "Controller.h"
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

std::string board_of(const Controller& controller) {
    std::ostringstream oss;
    controller.print(oss);
    return oss.str();
}

} // namespace

TEST_SUITE("Controller") {

// ---- outside the board ---------------------------------------------------

TEST_CASE("clicking outside the board is ignored") {
    SUBCASE("negative coordinate") {
        Controller controller(make_board());
        controller.click(-50, 50);
        CHECK_FALSE(controller.has_selection());
    }
    SUBCASE("past the board's far edge") {
        Controller controller(make_board());
        controller.click(350, 50); // board is 3 columns wide (0-299px)
        CHECK_FALSE(controller.has_selection());
    }
}

TEST_CASE("clicking outside the board cancels an active selection") {
    Controller controller(make_board());
    controller.click(50, 50); // select bR at (0,0)
    REQUIRE(controller.has_selection());

    controller.click(-50, 50); // outside the board
    CHECK_FALSE(controller.has_selection());
}

TEST_CASE("clicking on a degenerate 0x0 board is always ignored") {
    Controller controller(Parser::parse_board({}));
    controller.click(0, 0);
    controller.click(50, 50);
    CHECK_FALSE(controller.has_selection());
}

// ---- pixel-to-cell mapping -------------------------------------------------

TEST_CASE("pixel coordinates map to the containing cell, not just its center") {
    Controller controller(make_board());

    controller.click(100, 50); // crosses the x=100 boundary into cell (1,0) = bN
    REQUIRE(controller.has_selection());
    CHECK(controller.selected()->x == 1);
    CHECK(controller.selected()->y == 0);
}

// ---- selecting with nothing currently selected -----------------------------

TEST_CASE("clicking an empty cell with no selection is ignored") {
    Controller controller(make_board());
    controller.click(250, 50);  // (2,0) is empty
    controller.click(150, 150); // (1,1) is empty
    CHECK_FALSE(controller.has_selection());
}

TEST_CASE("clicking a piece with no selection selects it") {
    Controller controller(make_board());
    controller.click(50, 50); // (0,0) = bR
    REQUIRE(controller.has_selection());
    CHECK(controller.selected()->x == 0);
    CHECK(controller.selected()->y == 0);
}

// ---- replacing the selection ----------------------------------------------

TEST_CASE("clicking another friendly piece replaces the selection") {
    Controller controller(make_board());
    controller.click(50, 50);   // select bR at (0,0)
    controller.click(150, 50);  // bN at (1,0) is also black
    REQUIRE(controller.has_selection());
    CHECK(controller.selected()->x == 1);
    CHECK(controller.selected()->y == 0);
    CHECK(board_of(controller) == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("re-clicking the same selected piece keeps it selected") {
    Controller controller(make_board());
    controller.click(50, 50);
    controller.click(50, 50);
    REQUIRE(controller.has_selection());
    CHECK(controller.selected()->x == 0);
    CHECK(controller.selected()->y == 0);
}

// ---- move requests (settle after the travel time elapses) ------------------

TEST_CASE("clicking an empty cell while a piece is selected eventually moves it there") {
    Controller controller(make_board());
    controller.click(50, 50);  // select bR at (0,0)
    controller.click(50, 150); // (0,1) is empty; straight down is a legal rook move

    CHECK_FALSE(controller.has_selection());
    controller.wait(GameEngine::kDefaultMoveMsPerCell); // 1 cell of travel time
    CHECK(board_of(controller) == ". bN .\nbR . .\nwR . wN\n");
}

TEST_CASE("clicking an enemy piece while a piece is selected eventually captures it") {
    Controller controller(make_board());
    controller.click(50, 50);   // select bR at (0,0)
    controller.click(50, 250);  // wR at (0,2) is white; straight down the column

    CHECK_FALSE(controller.has_selection());
    controller.wait(2 * GameEngine::kDefaultMoveMsPerCell); // 2 cells of travel time
    CHECK(board_of(controller) == ". bN .\n. . .\nbR . wN\n");
}

TEST_CASE("the vacated source cell is empty once the move has arrived") {
    Controller controller(make_board());
    controller.click(50, 50);  // select bR at (0,0)
    controller.click(50, 150); // move down to (0,1)
    controller.wait(GameEngine::kDefaultMoveMsPerCell);

    controller.click(50, 50); // (0,0) is now empty; no selection to move
    CHECK_FALSE(controller.has_selection());
}

TEST_CASE("a cell with a move in flight cannot be reselected") {
    Controller controller(make_board());
    controller.click(50, 50);  // select bR at (0,0)
    controller.click(50, 150); // move down to (0,1); still in flight

    controller.click(50, 50); // (0,0) still shows bR, but it's mid-move
    CHECK_FALSE(controller.has_selection());
}

TEST_CASE("consecutive moves chain correctly once each one arrives") {
    Controller controller(make_board());
    controller.click(50, 50);  // select bR at (0,0)
    controller.click(50, 150); // move bR down to (0,1)
    controller.wait(GameEngine::kDefaultMoveMsPerCell);
    controller.wait(GameEngine::kCooldownMs); // let the just-arrived piece's cooldown elapse

    controller.click(50, 150); // re-select the piece that is now at (0,1)
    REQUIRE(controller.has_selection());
    CHECK(controller.selected()->x == 0);
    CHECK(controller.selected()->y == 1);

    controller.click(50, 250); // move down to (0,2), capturing wR
    controller.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK_FALSE(controller.has_selection());
    CHECK(board_of(controller) == ". bN .\n. . .\nbR . wN\n");
}

// ---- movement over time -----------------------------------------------------

TEST_CASE("the board still shows the piece at its original cell before arrival") {
    Controller controller(make_board());
    controller.click(50, 50);  // select bR at (0,0)
    controller.click(50, 150); // 1 cell of travel time is needed to arrive

    controller.wait(GameEngine::kDefaultMoveMsPerCell - 1); // one millisecond short
    CHECK(board_of(controller) == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("the piece appears at the destination once enough time has passed") {
    Controller controller(make_board());
    controller.click(50, 50);  // select bR at (0,0)
    controller.click(50, 150); // 1 cell of travel time is needed to arrive

    controller.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(controller) == ". bN .\nbR . .\nwR . wN\n");
}

// ---- a moving piece cannot be redirected; cooldown applies once it arrives ---

TEST_CASE("a piece already moving cannot be redirected to a new destination") {
    Controller controller(make_board());
    controller.click(50, 50);  // select bR at (0,0)
    controller.click(50, 150); // move down to (0,1); 1 cell of travel time

    // Attempt to redirect it mid-route by reselecting its (still visible)
    // origin cell; this fails, so there is no selection left to redirect.
    controller.click(50, 50);
    CHECK_FALSE(controller.has_selection());

    // Once the original move arrives, the piece is at its first destination
    // only; the redirect attempt had no effect.
    controller.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(controller) == ". bN .\nbR . .\nwR . wN\n");
}

TEST_CASE("a piece can be selected and moved again once its post-move cooldown elapses") {
    Controller controller(make_board());
    controller.click(50, 50);  // select bR at (0,0)
    controller.click(50, 150); // move down to (0,1)
    controller.wait(GameEngine::kDefaultMoveMsPerCell); // arrives; cooldown starts now
    controller.wait(GameEngine::kCooldownMs);            // cooldown elapses

    controller.click(50, 150); // select the piece now that its cooldown has elapsed
    REQUIRE(controller.has_selection());
    CHECK(controller.selected()->x == 0);
    CHECK(controller.selected()->y == 1);

    controller.click(50, 250); // immediately move it again, down to (0,2), capturing wR
    CHECK_FALSE(controller.has_selection());
    controller.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(controller) == ". bN .\n. . .\nbR . wN\n");
}

// ---- snapshot overlays the controller's own selection state ------------------

TEST_CASE("a snapshot taken with no selection has selected == nullopt") {
    Controller controller(make_board());
    CHECK(controller.snapshot().selected == std::nullopt);
}

TEST_CASE("a snapshot taken with an active selection reports the selected cell") {
    Controller controller(make_board());
    controller.click(50, 50); // select bR at (0,0)
    REQUIRE(controller.has_selection());

    CHECK(controller.snapshot().selected == Position{ 0, 0 });
}

TEST_CASE("a snapshot taken after the selection is cancelled reports nullopt again") {
    Controller controller(make_board());
    controller.click(50, 50);   // select bR at (0,0)
    controller.click(150, 150); // (1,1) is a diagonal move; illegal for a rook, cancels the selection
    REQUIRE_FALSE(controller.has_selection());

    CHECK(controller.snapshot().selected == std::nullopt);
}

// ---- illegal move attempts cancel the selection ------------------------------

TEST_CASE("a move that doesn't match the selected piece's shape cancels the selection") {
    Controller controller(make_board());
    controller.click(50, 50);   // select bR at (0,0)
    controller.click(150, 150); // (1,1) is a diagonal move; illegal for a rook

    CHECK_FALSE(controller.has_selection());
    CHECK(board_of(controller) == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("a blocked straight move cancels the selection") {
    Controller controller(Parser::parse_board({
        "bR .",
        "bN .",
        ".  .",
    }));
    controller.click(50, 50);  // select bR at (0,0)
    controller.click(50, 250); // (0,2) is past bN, which blocks the column at (0,1)

    CHECK_FALSE(controller.has_selection());
}

// ---- colliding moves on a shared route --------------------------------------

TEST_CASE("when two pieces attempt to swap places along the same route, whichever moved first wins") {
    SUBCASE("white moves first") {
        Controller controller(Parser::parse_board({ "wR . . bR" }));
        controller.click(50, 50);   // select wR at (0,0)
        controller.click(350, 50);  // move wR across to (3,0); 3 cells of travel time
        controller.click(350, 50);  // select bR at (3,0); it hasn't moved yet
        controller.click(50, 50);   // attempt to move bR back to (0,0); collides with wR's route

        // wR was scheduled first, so it wins: truncated to stop at the shared
        // cell (1,0); bR (the collision loser) is removed from the board.
        controller.wait(3 * GameEngine::kDefaultMoveMsPerCell);
        CHECK(board_of(controller) == ". wR . .\n");
    }

    SUBCASE("black moves first") {
        Controller controller(Parser::parse_board({ "wR . . bR" }));
        controller.click(350, 50);  // select bR at (3,0)
        controller.click(50, 50);   // move bR across to (0,0); 3 cells of travel time
        controller.click(50, 50);   // select wR at (0,0); it hasn't moved yet
        controller.click(350, 50);  // attempt to move wR back to (3,0); collides with bR's route

        // bR was scheduled first, so it wins: truncated to stop at the shared
        // cell (2,0); wR (the collision loser) is removed from the board.
        controller.wait(3 * GameEngine::kDefaultMoveMsPerCell);
        CHECK(board_of(controller) == ". . bR .\n");
    }
}

TEST_CASE("a move that collides with another in-flight move's route is still scheduled, clearing the selection") {
    Controller controller(Parser::parse_board({ "wR . . bR" }));
    controller.click(50, 50);  // select wR at (0,0)
    controller.click(350, 50); // move wR across to (3,0); still in flight
    controller.click(350, 50); // select bR at (3,0)
    controller.click(50, 50);  // move bR back to (0,0); collides with wR's route, but is still scheduled

    // Scheduling a colliding move clears the selection immediately; nothing
    // settles until wait() reaches the collision instant.
    CHECK_FALSE(controller.has_selection());
    CHECK(board_of(controller) == "wR . . bR\n");

    // wR was scheduled first, so it wins: truncated to stop at the shared
    // cell (1,0); bR (the collision loser) is removed from the board.
    controller.wait(3 * GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(controller) == ". wR . .\n");
}

TEST_CASE("a move onto another route's cell is rejected for its own illegality, not treated as a collision") {
    Controller controller(Parser::parse_board({
        ".  .  .  .",
        "wQ .  .  bK",
        ".  .  bP .",
        ".  .  .  .",
    }));
    controller.click(50, 150);  // select wQ at (0,1)
    controller.click(350, 150); // move wQ across to (3,1), capturing bK; 3 cells of travel time
    controller.wait(200);

    controller.click(250, 250); // select bP at (2,2)
    controller.click(250, 150); // (2,1) is on wQ's route, but this move is illegal anyway: backward for black

    controller.wait(3000);
    CHECK(board_of(controller) == ". . . .\n. . . wQ\n. . bP .\n. . . .\n");
}

// ---- friendly-piece landing ---------------------------------------------------

TEST_CASE("a knight cannot land on a friendly piece, even though the move shape is legal") {
    Controller controller(Parser::parse_board({
        ".  wP .",
        ".  .  .",
        "wN .  .",
    }));
    controller.click(50, 250); // select wN at (0,2)
    controller.click(150, 50); // attempt an L-shaped move onto wP at (1,0); same color

    controller.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(controller) == ". wP .\n. . .\nwN . .\n");
}

// ---- premoves that never became legal ------------------------------------------

TEST_CASE("a click on an unrelated empty cell after a failed redirect schedules nothing") {
    Controller controller(Parser::parse_board({ "wR . ." }));
    controller.click(50, 50);  // select wR at (0,0)
    controller.click(150, 50); // move to (1,0); 1 cell of travel time
    controller.click(50, 50);  // attempt to redirect; fails, nothing selected
    controller.click(250, 50); // (2,0) is empty and nothing is selected; ignored

    controller.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(controller) == ". wR .\n");
}

// ---- jumping and the selection ------------------------------------------------

TEST_CASE("selecting then jumping the same piece clears the selection") {
    Controller controller(Parser::parse_board({ "wR . ." }));
    controller.click(50, 50); // select wR at (0,0)
    REQUIRE(controller.has_selection());

    controller.jump(50, 50);  // jumping the selected piece drops the selection
    CHECK_FALSE(controller.has_selection());

    controller.click(250, 50); // nothing selected, empty cell -> no move scheduled
    controller.wait(GameEngine::kJumpDurationMs);
    CHECK(board_of(controller) == "wR . .\n");
}

// ---- per-color selection cursors ------------------------------------------

TEST_CASE("a color-restricted click on the caller's own piece selects it") {
    Controller controller(make_board());
    controller.click(50, 50, Color::b); // select bR at (0,0) as black
    REQUIRE(controller.has_selection(Color::b));
    CHECK(controller.selected(Color::b) == Position{ 0, 0 });
}

TEST_CASE("a color-restricted click on an enemy piece with no active selection is rejected") {
    Controller controller(make_board());
    controller.click(50, 250, Color::b); // wR at (0,2) belongs to white
    CHECK_FALSE(controller.has_selection(Color::b));
}

TEST_CASE("independent per-color cursors do not interfere when interleaved") {
    Controller controller(make_board());
    controller.click(50, 250, Color::w); // select wR at (0,2) as white
    controller.click(150, 50, Color::b); // select bN at (1,0) as black

    CHECK(controller.selected(Color::w) == Position{ 0, 2 });
    CHECK(controller.selected(Color::b) == Position{ 1, 0 });
}

TEST_CASE("a color-restricted click on a cell already selected by another color is rejected without disturbing it") {
    Controller controller(make_board());
    controller.click(50, 250, Color::w); // select wR at (0,2) as white
    controller.click(50, 250, Color::b); // wR belongs to white; rejected for black

    CHECK_FALSE(controller.has_selection(Color::b));
    CHECK(controller.selected(Color::w) == Position{ 0, 2 });
}

TEST_CASE("a full move scheduled and resolved under one color's cursor leaves another color's pending selection untouched") {
    Controller controller(make_board());
    controller.click(250, 250, Color::w); // select wN at (2,2) as white; left pending throughout

    controller.click(50, 50, Color::b);  // select bR at (0,0) as black
    controller.click(50, 150, Color::b); // move bR down to (0,1)

    controller.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(controller) == ". bN .\nbR . .\nwR . wN\n");
    CHECK(controller.selected(Color::w) == Position{ 2, 2 });
}

TEST_CASE("jumping under a color-restricted cursor clears only that color's own selection") {
    Controller controller(make_board());
    controller.click(50, 50, Color::b); // select bR at (0,0) as black
    REQUIRE(controller.has_selection(Color::b));

    controller.jump(50, 50, Color::b); // jumping the selected piece drops the black cursor
    CHECK_FALSE(controller.has_selection(Color::b));
}

TEST_CASE("a color-restricted jump on an enemy piece is rejected, leaving it normally selectable") {
    Controller controller(make_board());
    controller.jump(50, 250, Color::b); // wR at (0,2) belongs to white; rejected, no-op

    controller.click(50, 250, Color::w); // wR is still on the board and normally selectable by white
    CHECK(controller.has_selection(Color::w));
}

// ---- once the game is over -----------------------------------------------------

TEST_CASE("once the game is over, further clicks are ignored") {
    Controller controller(Parser::parse_board({ "wR . bK", "wN . ." }));
    controller.click(50, 50);   // select wR at (0,0)
    controller.click(250, 50);  // move across to (2,0), capturing bK
    controller.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    REQUIRE(controller.game_over());

    controller.click(50, 150); // attempt to select wN at (0,1); should be ignored
    CHECK_FALSE(controller.has_selection());
    CHECK(board_of(controller) == ". . wR\nwN . .\n");
}

TEST_CASE("once the game is over, a click outside the board still clears any stale selection") {
    Controller controller(Parser::parse_board({ "wR . bK", "wN . ." }));
    controller.click(50, 50);   // select wR at (0,0)
    controller.click(250, 50);  // move across to (2,0), capturing bK
    controller.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    REQUIRE(controller.game_over());

    controller.click(50, 150); // attempt to select wN; ignored, no selection created
    controller.click(-50, 50); // outside the board
    CHECK_FALSE(controller.has_selection());
}

}
