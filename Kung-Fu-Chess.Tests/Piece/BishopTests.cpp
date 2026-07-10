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

// ---- has_blockers -----------------------------------------------------------

TEST_CASE("has_blockers is true when a piece sits strictly between start and destination") {
    Bishop bishop;
    Board board(8, 8);
    board.place_at(3, 3, Cell{ Color::b, PieceType::P });
    CHECK(bishop.has_blockers(0, 0, 5, 5, board));
}

TEST_CASE("has_blockers is false when the diagonal is clear") {
    Bishop bishop;
    Board board(8, 8);
    CHECK_FALSE(bishop.has_blockers(0, 0, 5, 5, board));
}

TEST_CASE("has_blockers is false when only the destination cell is occupied") {
    Bishop bishop;
    Board board(8, 8);
    board.place_at(5, 5, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(bishop.has_blockers(0, 0, 5, 5, board));
}

// ---- capturing ---------------------------------------------------------------

TEST_CASE("capturing an enemy piece at the destination is legal") {
    Bishop bishop;
    Board board(8, 8);
    board.place_at(0, 0, Cell{ Color::w, PieceType::B });
    board.place_at(5, 5, Cell{ Color::b, PieceType::P });
    CHECK(bishop.is_available_move(0, 0, 5, 5, board));
}

TEST_CASE("capturing a piece of the same color at the destination is illegal") {
    Bishop bishop;
    Board board(8, 8);
    board.place_at(0, 0, Cell{ Color::w, PieceType::B });
    board.place_at(5, 5, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(bishop.is_available_move(0, 0, 5, 5, board));
}

}
