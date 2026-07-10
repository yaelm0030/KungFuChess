#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Piece.h"

TEST_SUITE("Piece::Knight") {

TEST_CASE("all eight L-shaped moves are legal") {
    Knight knight;
    Board board(8, 8);

    struct Offset { int dx; int dy; };
    const Offset offsets[] = {
        { 1, 2 }, { 2, 1 }, { -1, 2 }, { -2, 1 },
        { 1, -2 }, { 2, -1 }, { -1, -2 }, { -2, -1 },
    };

    for (const auto& o : offsets) {
        CAPTURE(o.dx);
        CAPTURE(o.dy);
        CHECK(knight.is_available_move(4, 4, 4 + o.dx, 4 + o.dy, board));
    }
}

TEST_CASE("the knight can jump over pieces blocking the in-between cells") {
    Knight knight;
    Board board(8, 8);
    board.place_at(4, 5, Cell{ Color::b, PieceType::P });
    board.place_at(5, 4, Cell{ Color::b, PieceType::P });
    CHECK(knight.is_available_move(4, 4, 5, 6, board));
}

TEST_CASE("moving one cell straight is illegal") {
    Knight knight;
    Board board(8, 8);
    CHECK_FALSE(knight.is_available_move(4, 4, 4, 5, board));
}

TEST_CASE("moving diagonally is illegal") {
    Knight knight;
    Board board(8, 8);
    CHECK_FALSE(knight.is_available_move(4, 4, 5, 5, board));
}

TEST_CASE("staying on the same cell is illegal") {
    Knight knight;
    Board board(8, 8);
    CHECK_FALSE(knight.is_available_move(4, 4, 4, 4, board));
}

// ---- has_blockers -----------------------------------------------------------

TEST_CASE("has_blockers is always false, since a knight jumps over pieces") {
    Knight knight;
    Board board(8, 8);
    board.place_at(4, 5, Cell{ Color::b, PieceType::P });
    board.place_at(5, 4, Cell{ Color::b, PieceType::P });
    CHECK_FALSE(knight.has_blockers(4, 4, 5, 6, board));
}

// ---- capturing ---------------------------------------------------------------

TEST_CASE("capturing an enemy piece at the destination is legal") {
    Knight knight;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::w, PieceType::N });
    board.place_at(5, 6, Cell{ Color::b, PieceType::P });
    CHECK(knight.is_available_move(4, 4, 5, 6, board));
}

TEST_CASE("capturing a piece of the same color at the destination is illegal") {
    Knight knight;
    Board board(8, 8);
    board.place_at(4, 4, Cell{ Color::w, PieceType::N });
    board.place_at(5, 6, Cell{ Color::w, PieceType::P });
    CHECK_FALSE(knight.is_available_move(4, 4, 5, 6, board));
}

}
