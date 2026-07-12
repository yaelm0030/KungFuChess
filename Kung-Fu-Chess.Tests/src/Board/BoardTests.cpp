#include "ThirdParty/doctest.h"

#include <stdexcept>

#include "Board.h"

TEST_SUITE("Board") {

TEST_CASE("get_at throws std::out_of_range for an out-of-bounds position") {
    Board board(2, 2);
    CHECK_THROWS_AS(board.get_at(-1, 0), std::out_of_range);
    CHECK_THROWS_AS(board.get_at(0, -1), std::out_of_range);
    CHECK_THROWS_AS(board.get_at(2, 0), std::out_of_range);
    CHECK_THROWS_AS(board.get_at(0, 2), std::out_of_range);
}

TEST_CASE("place_at throws std::out_of_range for an out-of-bounds position") {
    Board board(2, 2);
    CHECK_THROWS_AS(board.place_at(-1, 0, Cell{ Color::w, PieceType::P }), std::out_of_range);
    CHECK_THROWS_AS(board.place_at(2, 0, Cell{ Color::w, PieceType::P }), std::out_of_range);
}

TEST_CASE("clear_at throws std::out_of_range for an out-of-bounds position") {
    Board board(2, 2);
    CHECK_THROWS_AS(board.clear_at(-1, 0), std::out_of_range);
    CHECK_THROWS_AS(board.clear_at(0, 2), std::out_of_range);
}

}
