#include "ThirdParty/doctest.h"

#include "Constants.h"
#include "Parser.h"
#include "RealTimeArbiter.h"

TEST_SUITE("RealTimeArbiter::advance") {

// ---- head-on swap: priority follows scheduling order, not color/position --------

TEST_CASE("a head-on swap resolves in favor of whichever side was scheduled first") {
    SUBCASE("white scheduled first: wR wins, truncated to (1,0)") {
        Board board = Parser::parse_board({ "wR . . bR" });
        RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
        arbiter.schedule_move(Position{ 0, 0 }, Position{ 3, 0 }, *board.get_at(0, 0));
        arbiter.schedule_move(Position{ 3, 0 }, Position{ 0, 0 }, *board.get_at(3, 0));

        CHECK_FALSE(arbiter.advance(2 * constants::kDefaultMoveMsPerCell));
        CHECK(Parser::board_to_string(board) == ". wR . .");
        CHECK(board.get_at(1, 0)->cooldown_end_ms == arbiter.clock_ms() + constants::kCooldownMs);
    }

    SUBCASE("black scheduled first: bR wins, truncated to (2,0)") {
        Board board = Parser::parse_board({ "wR . . bR" });
        RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
        arbiter.schedule_move(Position{ 3, 0 }, Position{ 0, 0 }, *board.get_at(3, 0));
        arbiter.schedule_move(Position{ 0, 0 }, Position{ 3, 0 }, *board.get_at(0, 0));

        CHECK_FALSE(arbiter.advance(2 * constants::kDefaultMoveMsPerCell));
        CHECK(Parser::board_to_string(board) == ". . bR .");
        CHECK(board.get_at(2, 0)->cooldown_end_ms == arbiter.clock_ms() + constants::kCooldownMs);
    }
}

TEST_CASE("a head-on swap is untouched until the collision instant is actually due") {
    Board board = Parser::parse_board({ "wR . . bR" });
    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 3, 0 }, *board.get_at(0, 0));
    arbiter.schedule_move(Position{ 3, 0 }, Position{ 0, 0 }, *board.get_at(3, 0));

    arbiter.advance(2 * constants::kDefaultMoveMsPerCell - 1); // one millisecond short
    CHECK(arbiter.is_moving(0, 0));
    CHECK(arbiter.is_moving(3, 0));
    CHECK(Parser::board_to_string(board) == "wR . . bR");
}

// ---- a chain of collisions resolves fully within a single advance() call ------

TEST_CASE("resolving one collision can expose a second, and both resolve in one advance() call") {
    // X (0,0)->(3,0) and Y (3,0)->(0,0) collide at (1,0), same as the swap
    // above: X wins (scheduled first) and is truncated to dest=(1,0). Z
    // (1,1)->(1,0) independently reaches that same cell at t=1000, so once
    // Y is out of the way, X (still winning on sequence) collides with Z
    // there too - a second resolution only reachable by rescanning after
    // the first removal.
    Board board(4, 2);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R }); // X
    board.place_at(3, 0, Cell{ Color::b, PieceType::R }); // Y
    board.place_at(1, 1, Cell{ Color::b, PieceType::B }); // Z

    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 3, 0 }, *board.get_at(0, 0)); // X; sequence 0
    arbiter.schedule_move(Position{ 3, 0 }, Position{ 0, 0 }, *board.get_at(3, 0)); // Y; sequence 1
    arbiter.schedule_move(Position{ 1, 1 }, Position{ 1, 0 }, *board.get_at(1, 1)); // Z; sequence 2

    CHECK_FALSE(arbiter.advance(2 * constants::kDefaultMoveMsPerCell));
    CHECK(Parser::board_to_string(board) == ". wR . .\n. . . .");
}

// ---- crossing (non-collinear) paths --------------------------------------------

TEST_CASE("crossing diagonal paths collide at their single shared cell") {
    // wB (0,0)->(4,4) and bB (4,0)->(0,4): an X shape sharing only (2,2).
    Board board(5, 5);
    board.place_at(0, 0, Cell{ Color::w, PieceType::B });
    board.place_at(4, 0, Cell{ Color::b, PieceType::B });

    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 4, 4 }, *board.get_at(0, 0)); // wB; sequence 0
    arbiter.schedule_move(Position{ 4, 0 }, Position{ 0, 4 }, *board.get_at(4, 0)); // bB; sequence 1

    CHECK_FALSE(arbiter.advance(2 * constants::kDefaultMoveMsPerCell)); // both reach (2,2) at t=2000
    CHECK(Parser::board_to_string(board) ==
          ". . . . .\n. . . . .\n. . wB . .\n. . . . .\n. . . . .");
}

// ---- negative control: disjoint paths never collide ----------------------------

TEST_CASE("two pending moves whose paths share no cell settle normally") {
    Board board = Parser::parse_board({
        ".  . .",
        ".  . .",
        "wR . bR",
    });
    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 2 }, Position{ 0, 0 }, *board.get_at(0, 2)); // wR straight up
    arbiter.schedule_move(Position{ 2, 2 }, Position{ 2, 0 }, *board.get_at(2, 2)); // bR straight up

    CHECK_FALSE(arbiter.advance(2 * constants::kDefaultMoveMsPerCell));
    CHECK(Parser::board_to_string(board) == "wR . bR\n. . .\n. . .");
}

// ---- a knight is exempt from collisions, in either scheduling role -------------

TEST_CASE("a knight completes its own route untouched, regardless of scheduling order") {
    // wN at (0,0) -> (2,1) (L-shape, 2 cells of travel time), bR at (2,4) -> (2,1)
    // (3 cells of travel time): both target (2,1), but they arrive at different
    // times, so this is an ordinary capture-on-arrival, not a collision.
    auto run_case = [](bool knight_scheduled_first) {
        Board board(3, 5);
        board.place_at(0, 0, Cell{ Color::w, PieceType::N });
        board.place_at(2, 4, Cell{ Color::b, PieceType::R });

        RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
        if (knight_scheduled_first) {
            arbiter.schedule_move(Position{ 0, 0 }, Position{ 2, 1 }, *board.get_at(0, 0));
            arbiter.schedule_move(Position{ 2, 4 }, Position{ 2, 1 }, *board.get_at(2, 4));
        } else {
            arbiter.schedule_move(Position{ 2, 4 }, Position{ 2, 1 }, *board.get_at(2, 4));
            arbiter.schedule_move(Position{ 0, 0 }, Position{ 2, 1 }, *board.get_at(0, 0));
        }

        arbiter.advance(2 * constants::kDefaultMoveMsPerCell); // the knight's full route completes
        CHECK(board.get_at(2, 1)->type == PieceType::N);
        CHECK_FALSE(board.get_at(0, 0).has_value()); // the knight's own start is vacated

        arbiter.advance(constants::kDefaultMoveMsPerCell); // the rook's full route completes next
        CHECK(board.get_at(2, 1)->type == PieceType::R); // ordinary capture-on-arrival
        CHECK_FALSE(board.get_at(2, 4).has_value());     // the rook's own start is vacated
    };

    SUBCASE("knight scheduled first") {
        run_case(true);
    }
    SUBCASE("knight scheduled second") {
        run_case(false);
    }
}

// ---- a King ending up on either side of a collision ends the game -------------

TEST_CASE("a collision-truncated move that lands on an enemy king reports a king capture") {
    Board board = Parser::parse_board({ "wR bK . bR" }); // bK sits right on the swap's collision cell
    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 3, 0 }, *board.get_at(0, 0)); // wR; sequence 0
    arbiter.schedule_move(Position{ 3, 0 }, Position{ 0, 0 }, *board.get_at(3, 0)); // bR; sequence 1

    CHECK(arbiter.advance(2 * constants::kDefaultMoveMsPerCell)); // king captured
    CHECK(Parser::board_to_string(board) == ". wR . .");
}

TEST_CASE("a King removed as a collision loser reports a king capture, same as a normal capture") {
    Board board = Parser::parse_board({ "wR . . bK" }); // bK itself loses the swap this time
    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 3, 0 }, *board.get_at(0, 0)); // wR; sequence 0
    arbiter.schedule_move(Position{ 3, 0 }, Position{ 0, 0 }, *board.get_at(3, 0)); // bK; sequence 1

    CHECK(arbiter.advance(2 * constants::kDefaultMoveMsPerCell)); // king captured, as the collision loser
    CHECK(Parser::board_to_string(board) == ". wR . .");
}

// ---- friendly collisions: the later-scheduled mover yields, neither is removed ---

TEST_CASE("a later-scheduled friendly rook truncates one cell short of an earlier friendly rook's landing") {
    Board board(6, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R }); // A
    board.place_at(5, 0, Cell{ Color::w, PieceType::R }); // B; same color as A

    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 2, 0 }, *board.get_at(0, 0)); // A; sequence 0
    arbiter.schedule_move(Position{ 5, 0 }, Position{ 2, 0 }, *board.get_at(5, 0)); // B; sequence 1

    // A (scheduled first) is untouched and lands normally at (2,0); B (scheduled
    // later) must yield, stopping one cell short of A's landing cell at (3,0).
    // Neither is removed - unlike a hostile collision, both stay on the board.
    CHECK_FALSE(arbiter.advance(3 * constants::kDefaultMoveMsPerCell));
    CHECK(Parser::board_to_string(board) == ". . wR wR . .");
}

TEST_CASE("crossing diagonal paths, same color: the higher-sequence bishop stops one cell short") {
    // wB (0,0)->(4,4) and wB (4,0)->(0,4): the same X shape as the hostile
    // crossing test, but same color this time.
    Board board(5, 5);
    board.place_at(0, 0, Cell{ Color::w, PieceType::B }); // A; sequence 0
    board.place_at(4, 0, Cell{ Color::w, PieceType::B }); // B; sequence 1

    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 4, 4 }, *board.get_at(0, 0));
    arbiter.schedule_move(Position{ 4, 0 }, Position{ 0, 4 }, *board.get_at(4, 0));

    // Both reach the shared cell (2,2) at t=2000. B (higher sequence) yields,
    // truncated - along its own path - to the cell just before it, (3,1).
    arbiter.advance(2 * constants::kDefaultMoveMsPerCell);
    CHECK(board.get_at(3, 1)->type == PieceType::B);
    CHECK_FALSE(board.get_at(4, 0).has_value());
    CHECK(board.get_at(0, 0).has_value()); // A hasn't arrived yet; untouched by the yield

    // A (untouched) continues to its own original destination normally.
    arbiter.advance(2 * constants::kDefaultMoveMsPerCell);
    CHECK(board.get_at(4, 4)->type == PieceType::B);
    CHECK_FALSE(board.get_at(0, 0).has_value());
}

TEST_CASE("a friendly collision is untouched until the collision instant is actually due") {
    Board board(5, 5);
    board.place_at(0, 0, Cell{ Color::w, PieceType::B });
    board.place_at(4, 0, Cell{ Color::w, PieceType::B });

    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 4, 4 }, *board.get_at(0, 0));
    arbiter.schedule_move(Position{ 4, 0 }, Position{ 0, 4 }, *board.get_at(4, 0));

    arbiter.advance(2 * constants::kDefaultMoveMsPerCell - 1); // one millisecond short
    CHECK(arbiter.is_moving(0, 0));
    CHECK(arbiter.is_moving(4, 0));
    CHECK(Parser::board_to_string(board) ==
          "wB . . . wB\n. . . . .\n. . . . .\n. . . . .\n. . . . .");
}

TEST_CASE("two friendly pending moves whose paths share no cell settle normally") {
    Board board = Parser::parse_board({
        ".  . .",
        ".  . .",
        "wR . wR",
    });
    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 2 }, Position{ 0, 0 }, *board.get_at(0, 2)); // wR straight up
    arbiter.schedule_move(Position{ 2, 2 }, Position{ 2, 0 }, *board.get_at(2, 2)); // wR straight up

    CHECK_FALSE(arbiter.advance(2 * constants::kDefaultMoveMsPerCell));
    CHECK(Parser::board_to_string(board) == "wR . wR\n. . .\n. . .");
}

// ---- a knight loses its universal exemption only for its own destination -------

TEST_CASE("a friendly knight still passes through a shared cell that isn't its own destination") {
    // wN (2,0)->(0,1): an L-shape whose only non-destination path cell is its
    // own start, (2,0). wR (1,0)->(3,0) passes through that same cell as an
    // intermediate step (not the rook's destination either). Since (2,0) is
    // not the knight's destination, it stays fully exempt, same color or not.
    Board board(4, 2);
    board.place_at(2, 0, Cell{ Color::w, PieceType::N });
    board.place_at(1, 0, Cell{ Color::w, PieceType::R });

    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 2, 0 }, Position{ 0, 1 }, *board.get_at(2, 0));
    arbiter.schedule_move(Position{ 1, 0 }, Position{ 3, 0 }, *board.get_at(1, 0));

    arbiter.advance(2 * constants::kDefaultMoveMsPerCell);
    CHECK(board.get_at(0, 1)->type == PieceType::N);
    CHECK(board.get_at(3, 0)->type == PieceType::R);
    CHECK_FALSE(board.get_at(2, 0).has_value());
    CHECK_FALSE(board.get_at(1, 0).has_value());
}

TEST_CASE("a friendly piece yields rather than let a knight land on top of it, and vice versa") {
    // wN (0,0)->(2,1) (sequence 0) and wR (2,4)->(2,1) (sequence 1) both
    // target (2,1) - the knight's own destination. Unlike the hostile/
    // different-arrival-time case, the knight is not exempt here: it wins on
    // sequence and lands normally, so the rook must yield instead of
    // silently overwriting it via an ordinary capture-on-arrival.
    Board board(3, 5);
    board.place_at(0, 0, Cell{ Color::w, PieceType::N });
    board.place_at(2, 4, Cell{ Color::w, PieceType::R });

    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 2, 1 }, *board.get_at(0, 0)); // wN; sequence 0
    arbiter.schedule_move(Position{ 2, 4 }, Position{ 2, 1 }, *board.get_at(2, 4)); // wR; sequence 1

    arbiter.advance(3 * constants::kDefaultMoveMsPerCell);
    CHECK(board.get_at(2, 1)->type == PieceType::N); // the knight lands as normal
    CHECK(board.get_at(2, 2)->type == PieceType::R); // the rook stops one cell short instead
    CHECK_FALSE(board.get_at(0, 0).has_value());
    CHECK_FALSE(board.get_at(2, 4).has_value());
}

// ---- zero-length yield: already adjacent when the collision becomes due -------

TEST_CASE("a friendly piece already adjacent to the shared cell yields as a no-op, not a truncated landing") {
    Board board(4, 2);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R }); // A; sequence 0
    board.place_at(2, 1, Cell{ Color::w, PieceType::R }); // B; sequence 1

    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 3, 0 }, *board.get_at(0, 0)); // A
    arbiter.schedule_move(Position{ 2, 1 }, Position{ 2, 0 }, *board.get_at(2, 1)); // B; one cell from (2,0)

    // The shared cell (2,0) is due at t=2000, which is already B's very next
    // (and only) step - so "one cell short" is B's own start: a no-op.
    CHECK_FALSE(arbiter.advance(2 * constants::kDefaultMoveMsPerCell));
    CHECK_FALSE(arbiter.is_moving(2, 1));               // B's move was dropped, not delayed
    CHECK(board.get_at(2, 1)->cooldown_end_ms == 0);    // never landed, so never cooldown-stamped
    CHECK_FALSE(board.get_at(2, 0).has_value());        // B never actually got there
    CHECK(arbiter.is_moving(0, 0));                     // A is untouched, still travelling normally
}

TEST_CASE("a normal (non-zero-length) friendly yield still gets cooldown-stamped on landing") {
    Board board(6, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R }); // A; sequence 0
    board.place_at(5, 0, Cell{ Color::w, PieceType::R }); // B; sequence 1

    RealTimeArbiter arbiter(board, constants::kDefaultMoveMsPerCell);
    arbiter.schedule_move(Position{ 0, 0 }, Position{ 2, 0 }, *board.get_at(0, 0));
    arbiter.schedule_move(Position{ 5, 0 }, Position{ 2, 0 }, *board.get_at(5, 0));

    arbiter.advance(3 * constants::kDefaultMoveMsPerCell);
    // B yielded and stopped at (3,0); its landing there is cooldown-stamped
    // exactly like any other settled move.
    CHECK(board.get_at(3, 0)->cooldown_end_ms == arbiter.clock_ms() + constants::kCooldownMs);
}

}
