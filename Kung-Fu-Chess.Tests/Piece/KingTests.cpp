#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Piece.h"

namespace {

Board make_empty_board() {
    return Board(8, 8);
}

} // namespace

TEST_SUITE("Piece::King") {

TEST_CASE("moving one cell in any of the eight directions is legal") {
    King king;
    Board board = make_empty_board();

    struct Offset { int dx; int dy; };
    const Offset offsets[] = {
        { -1, -1 }, { 0, -1 }, { 1, -1 },
        { -1,  0 },            { 1,  0 },
        { -1,  1 }, { 0,  1 }, { 1,  1 },
    };

    for (const auto& o : offsets) {
        CAPTURE(o.dx);
        CAPTURE(o.dy);
        CHECK(king.is_available_move(4, 4, 4 + o.dx, 4 + o.dy, board));
    }
}

TEST_CASE("staying on the same cell is illegal") {
    King king;
    Board board = make_empty_board();
    CHECK_FALSE(king.is_available_move(4, 4, 4, 4, board));
}

TEST_CASE("moving two cells in a straight line is illegal") {
    King king;
    Board board = make_empty_board();
    CHECK_FALSE(king.is_available_move(4, 4, 4, 6, board));
}

TEST_CASE("moving two cells diagonally is illegal") {
    King king;
    Board board = make_empty_board();
    CHECK_FALSE(king.is_available_move(4, 4, 6, 6, board));
}

TEST_CASE("a knight-shaped move is illegal") {
    King king;
    Board board = make_empty_board();
    CHECK_FALSE(king.is_available_move(4, 4, 5, 6, board));
}

}
