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

// ---- two-cell moves are illegal -----------------------------------------------

TEST_CASE("a white pawn cannot move two cells forward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 6, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 6, 4, 4, board));
}

TEST_CASE("a black pawn cannot move two cells forward") {
    Pawn pawn;
    Board board(8, 8);
    board.place_at(4, 1, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(pawn.is_available_move(4, 1, 4, 3, board));
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

TEST_CASE("has_blockers is always false, since a pawn only moves one cell") {
    Pawn pawn;
    Board board(8, 8);
    CHECK_FALSE(pawn.has_blockers(4, 4, 4, 3, board));
}

}
