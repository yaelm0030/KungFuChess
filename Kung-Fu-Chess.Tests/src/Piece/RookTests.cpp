#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Piece.h"

TEST_SUITE("Piece::Rook") {

TEST_CASE("moving along a clear row is legal") {
    Rook rook;
    Board board(8, 8);
    CHECK(rook.is_available_move(0, 0, 5, 0, board));
}

TEST_CASE("moving along a clear column is legal") {
    Rook rook;
    Board board(8, 8);
    CHECK(rook.is_available_move(0, 0, 0, 5, board));
}

TEST_CASE("moving backwards along a clear row or column is legal") {
    Rook rook;
    Board board(8, 8);
    CHECK(rook.is_available_move(5, 0, 0, 0, board));
    CHECK(rook.is_available_move(0, 5, 0, 0, board));
}

TEST_CASE("a piece blocking the path makes the move illegal") {
    Rook rook;
    Board board(8, 8);
    board.place_at(3, 0, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(rook.is_available_move(0, 0, 5, 0, board));
}

TEST_CASE("a piece on the destination cell only does not block the move") {
    Rook rook;
    Board board(8, 8);
    board.place_at(5, 0, Cell{ Color::b, PieceType::P });
    CHECK(rook.is_available_move(0, 0, 5, 0, board));
}

TEST_CASE("moving diagonally is illegal") {
    Rook rook;
    Board board(8, 8);
    CHECK_FALSE(rook.is_available_move(0, 0, 3, 3, board));
}

TEST_CASE("staying on the same cell is illegal") {
    Rook rook;
    Board board(8, 8);
    CHECK_FALSE(rook.is_available_move(4, 4, 4, 4, board));
}

// ---- has_blockers -----------------------------------------------------------

TEST_CASE("has_blockers is true when a piece sits strictly between start and destination") {
    Rook rook;
    Board board(8, 8);
    board.place_at(3, 0, Cell{ Color::b, PieceType::P });
    CHECK(rook.has_blockers(0, 0, 5, 0, board));
}

TEST_CASE("has_blockers is false when the path is clear") {
    Rook rook;
    Board board(8, 8);
    CHECK_FALSE(rook.has_blockers(0, 0, 5, 0, board));
}

TEST_CASE("has_blockers is false when only the destination cell is occupied") {
    Rook rook;
    Board board(8, 8);
    board.place_at(5, 0, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(rook.has_blockers(0, 0, 5, 0, board));
}

// ---- capturing ---------------------------------------------------------------

TEST_CASE("capturing an enemy piece at the destination is legal") {
    Rook rook;
    Board board(8, 8);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(5, 0, Cell{ Color::b, PieceType::P });
    CHECK(rook.is_available_move(0, 0, 5, 0, board));
}

TEST_CASE("capturing a piece of the same color at the destination is illegal") {
    Rook rook;
    Board board(8, 8);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(5, 0, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(rook.is_available_move(0, 0, 5, 0, board));
}

}
