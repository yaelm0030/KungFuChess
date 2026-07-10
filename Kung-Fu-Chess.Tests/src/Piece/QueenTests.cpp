#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Piece.h"

TEST_SUITE("Piece::Queen") {

TEST_CASE("moving along a clear row, column, or diagonal is legal") {
    Queen queen;
    Board board(8, 8);
    CHECK(queen.is_available_move(0, 0, 5, 0, board));
    CHECK(queen.is_available_move(0, 0, 0, 5, board));
    CHECK(queen.is_available_move(0, 0, 5, 5, board));
}

TEST_CASE("a piece blocking a straight path makes the move illegal") {
    Queen queen;
    Board board(8, 8);
    board.place_at(3, 0, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(queen.is_available_move(0, 0, 5, 0, board));
}

TEST_CASE("a piece blocking a diagonal path makes the move illegal") {
    Queen queen;
    Board board(8, 8);
    board.place_at(3, 3, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(queen.is_available_move(0, 0, 5, 5, board));
}

TEST_CASE("a piece on the destination cell only does not block the move") {
    Queen queen;
    Board board(8, 8);
    board.place_at(5, 5, Cell{ Color::b, PieceType::P });
    CHECK(queen.is_available_move(0, 0, 5, 5, board));
}

TEST_CASE("a knight-shaped move is illegal") {
    Queen queen;
    Board board(8, 8);
    CHECK_FALSE(queen.is_available_move(4, 4, 5, 6, board));
}

TEST_CASE("staying on the same cell is illegal") {
    Queen queen;
    Board board(8, 8);
    CHECK_FALSE(queen.is_available_move(4, 4, 4, 4, board));
}

// ---- has_blockers -----------------------------------------------------------

TEST_CASE("has_blockers is true when a piece sits strictly between start and destination") {
    Queen queen;
    Board board(8, 8);
    board.place_at(3, 0, Cell{ Color::b, PieceType::P });
    CHECK(queen.has_blockers(0, 0, 5, 0, board));
}

TEST_CASE("has_blockers is false when the row, column, or diagonal is clear") {
    Queen queen;
    Board board(8, 8);
    CHECK_FALSE(queen.has_blockers(0, 0, 5, 0, board));
    CHECK_FALSE(queen.has_blockers(0, 0, 0, 5, board));
    CHECK_FALSE(queen.has_blockers(0, 0, 5, 5, board));
}

// ---- capturing ---------------------------------------------------------------

TEST_CASE("capturing an enemy piece at the destination is legal") {
    Queen queen;
    Board board(8, 8);
    board.place_at(0, 0, Cell{ Color::w, PieceType::Q });
    board.place_at(5, 5, Cell{ Color::b, PieceType::P });
    CHECK(queen.is_available_move(0, 0, 5, 5, board));
}

TEST_CASE("capturing a piece of the same color at the destination is illegal") {
    Queen queen;
    Board board(8, 8);
    board.place_at(0, 0, Cell{ Color::w, PieceType::Q });
    board.place_at(5, 5, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(queen.is_available_move(0, 0, 5, 5, board));
}

}
