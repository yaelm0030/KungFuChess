#include "ThirdParty/doctest.h"

#include "Piece.h"

TEST_SUITE("Piece::can_pass_through_units") {

TEST_CASE("a knight can pass through units") {
    Knight knight;
    CHECK(knight.can_pass_through_units());
}

TEST_CASE("a king cannot pass through units") {
    King king;
    CHECK_FALSE(king.can_pass_through_units());
}

TEST_CASE("a queen cannot pass through units") {
    Queen queen;
    CHECK_FALSE(queen.can_pass_through_units());
}

TEST_CASE("a rook cannot pass through units") {
    Rook rook;
    CHECK_FALSE(rook.can_pass_through_units());
}

TEST_CASE("a bishop cannot pass through units") {
    Bishop bishop;
    CHECK_FALSE(bishop.can_pass_through_units());
}

TEST_CASE("a pawn cannot pass through units") {
    Pawn pawn;
    CHECK_FALSE(pawn.can_pass_through_units());
}

TEST_CASE("the flyweight knight instance served by the factory can pass through units") {
    CHECK(PieceFactory::get_piece(PieceType::N)->can_pass_through_units());
}

}
