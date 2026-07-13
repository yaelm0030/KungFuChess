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

}
