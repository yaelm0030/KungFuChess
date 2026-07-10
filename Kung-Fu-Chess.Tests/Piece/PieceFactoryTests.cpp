#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Piece.h"

TEST_SUITE("PieceFactory::get_piece") {

TEST_CASE("each implemented piece type resolves to an instance of the matching class") {
    CHECK(dynamic_cast<const King*>(PieceFactory::get_piece(PieceType::K)) != nullptr);
    CHECK(dynamic_cast<const Queen*>(PieceFactory::get_piece(PieceType::Q)) != nullptr);
    CHECK(dynamic_cast<const Rook*>(PieceFactory::get_piece(PieceType::R)) != nullptr);
    CHECK(dynamic_cast<const Bishop*>(PieceFactory::get_piece(PieceType::B)) != nullptr);
    CHECK(dynamic_cast<const Knight*>(PieceFactory::get_piece(PieceType::N)) != nullptr);
    CHECK(dynamic_cast<const Pawn*>(PieceFactory::get_piece(PieceType::P)) != nullptr);
}

TEST_CASE("repeated calls for the same type return the same shared instance") {
    CHECK(PieceFactory::get_piece(PieceType::K) == PieceFactory::get_piece(PieceType::K));
    CHECK(PieceFactory::get_piece(PieceType::R) == PieceFactory::get_piece(PieceType::R));
    CHECK(PieceFactory::get_piece(PieceType::P) == PieceFactory::get_piece(PieceType::P));
}

}
