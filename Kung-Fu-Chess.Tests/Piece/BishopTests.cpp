#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Piece.h"

TEST_SUITE("Piece::Bishop") {

TEST_CASE("moving along a clear diagonal is legal") {
    Bishop bishop;
    Board board(8, 8);
    CHECK(bishop.is_available_move(0, 0, 5, 5, board));
}

TEST_CASE("moving backwards along a clear diagonal is legal") {
    Bishop bishop;
    Board board(8, 8);
    CHECK(bishop.is_available_move(5, 5, 0, 0, board));
}

TEST_CASE("moving along the anti-diagonal is legal") {
    Bishop bishop;
    Board board(8, 8);
    CHECK(bishop.is_available_move(0, 5, 5, 0, board));
}

TEST_CASE("a piece blocking the diagonal makes the move illegal") {
    Bishop bishop;
    Board board(8, 8);
    board.place_at(3, 3, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(bishop.is_available_move(0, 0, 5, 5, board));
}

TEST_CASE("a piece on the destination cell only does not block the move") {
    Bishop bishop;
    Board board(8, 8);
    board.place_at(5, 5, Cell{ Color::b, PieceType::P });
    CHECK(bishop.is_available_move(0, 0, 5, 5, board));
}

TEST_CASE("moving in a straight line is illegal") {
    Bishop bishop;
    Board board(8, 8);
    CHECK_FALSE(bishop.is_available_move(0, 0, 5, 0, board));
}

TEST_CASE("staying on the same cell is illegal") {
    Bishop bishop;
    Board board(8, 8);
    CHECK_FALSE(bishop.is_available_move(4, 4, 4, 4, board));
}

}
