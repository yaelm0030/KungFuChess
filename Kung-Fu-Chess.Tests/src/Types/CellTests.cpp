#include "ThirdParty/doctest.h"

#include "Types.h"

TEST_SUITE("Cell::is_on_cooldown") {

TEST_CASE("a cell with no cooldown set is never on cooldown") {
    Cell cell{ Color::w, PieceType::P };
    CHECK_FALSE(cell.is_on_cooldown(0));
}

TEST_CASE("a cell is on cooldown strictly before its cooldown end time") {
    Cell cell{ Color::w, PieceType::P, 1000 };
    CHECK(cell.is_on_cooldown(999));
}

TEST_CASE("a cell is no longer on cooldown exactly at its cooldown end time") {
    Cell cell{ Color::w, PieceType::P, 1000 };
    CHECK_FALSE(cell.is_on_cooldown(1000));
}

TEST_CASE("a cell is no longer on cooldown after its cooldown end time") {
    Cell cell{ Color::w, PieceType::P, 1000 };
    CHECK_FALSE(cell.is_on_cooldown(1001));
}

}
