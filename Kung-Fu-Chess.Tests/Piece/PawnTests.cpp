#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Piece.h"

TEST_SUITE("Piece::Pawn") {

// ---- direction of travel -----------------------------------------------------

TEST_CASE("a white pawn moves upward, toward decreasing y") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::w, PieceType::P });
    CHECK(pawn.is_available_move(4, 4, 4, 3, board));
}

TEST_CASE("a black pawn moves downward, toward increasing y") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::b, PieceType::P });
    CHECK(pawn.is_available_move(4, 4, 4, 5, board));
}

TEST_CASE("a white pawn cannot move downward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 4, 4, 5, board));
}

TEST_CASE("a black pawn cannot move upward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 4, 4, 3, board));
}

// ---- two-cell moves from the home row -------------------------------------------

TEST_CASE("a white pawn on its home row can move two cells forward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 7, Cell{ Color::w, PieceType::P });
    CHECK(pawn.is_available_move(4, 7, 4, 5, board));
}

TEST_CASE("a black pawn on its home row can move two cells forward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 0, Cell{ Color::b, PieceType::P });
    CHECK(pawn.is_available_move(4, 0, 4, 2, board));
}

TEST_CASE("a white pawn off its home row cannot move two cells forward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 6, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 6, 4, 4, board));
}

TEST_CASE("a black pawn off its home row cannot move two cells forward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 1, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 1, 4, 3, board));
}

TEST_CASE("a two-cell move is blocked by a piece one cell ahead") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 7, Cell{ Color::w, PieceType::P });
    board.place_at(4, 6, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 7, 4, 5, board));
}

TEST_CASE("a two-cell move is blocked by a piece on the destination cell") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 7, Cell{ Color::w, PieceType::P });
    board.place_at(4, 5, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 7, 4, 5, board));
}

TEST_CASE("a two-cell move sideways or on the diagonal is illegal") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 7, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 7, 6, 7, board));
    CHECK_FALSE(pawn.is_available_move(4, 7, 6, 5, board));
}

// ---- has_blockers on the two-cell move -------------------------------------------

TEST_CASE("has_blockers is true for a two-cell move with a piece in the way") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 7, Cell{ Color::w, PieceType::P });
    board.place_at(4, 6, Cell{ Color::b, PieceType::P });
    CHECK(pawn.has_blockers(4, 7, 4, 5, board));
}

TEST_CASE("has_blockers is false for a two-cell move with a clear path") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 7, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(pawn.has_blockers(4, 7, 4, 5, board));
}

// ---- diagonal captures ---------------------------------------------------------

TEST_CASE("a white pawn captures an enemy piece diagonally forward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::w, PieceType::P });
    board.place_at(5, 3, Cell{ Color::b, PieceType::P });
    CHECK(pawn.is_available_move(4, 4, 5, 3, board));
}

TEST_CASE("a black pawn captures an enemy piece diagonally forward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::b, PieceType::P });
    board.place_at(3, 5, Cell{ Color::w, PieceType::P });
    CHECK(pawn.is_available_move(4, 4, 3, 5, board));
}

TEST_CASE("a diagonal move onto an empty cell is illegal") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 4, 5, 3, board));
}

TEST_CASE("a diagonal move onto a piece of the same color is illegal") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::w, PieceType::P });
    board.place_at(5, 3, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 4, 5, 3, board));
}

// ---- capturing straight ahead is illegal ----------------------------------------

TEST_CASE("a pawn cannot capture a piece directly ahead of it") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::w, PieceType::P });
    board.place_at(4, 3, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 4, 4, 3, board));
}

// ---- has_blockers -----------------------------------------------------------

TEST_CASE("has_blockers returns false for a normal one-cell move") {
    Pawn pawn;
    Board board(8, 8);
    CHECK_FALSE(pawn.has_blockers(4, 4, 4, 3, board));
}

}
