#include "ThirdParty/doctest.h"

#include "BoardMapper.h"
#include "Constants.h"

TEST_SUITE("BoardMapper") {

TEST_CASE("a pixel maps to the cell containing it") {
    std::optional<Position> cell = BoardMapper::pixel_to_cell(150, 250, 8, 8); // -> cell (1, 2)
    REQUIRE(cell.has_value());
    CHECK(cell->x == 1);
    CHECK(cell->y == 2);
}

TEST_CASE("a pixel exactly on a cell boundary maps to the next cell") {
    std::optional<Position> cell = BoardMapper::pixel_to_cell(constants::kCellSizePx, 0, 8, 8);
    REQUIRE(cell.has_value());
    CHECK(cell->x == 1);
    CHECK(cell->y == 0);
}

TEST_CASE("a pixel one short of a cell boundary stays in the previous cell") {
    std::optional<Position> cell = BoardMapper::pixel_to_cell(constants::kCellSizePx - 1, 0, 8, 8);
    REQUIRE(cell.has_value());
    CHECK(cell->x == 0);
    CHECK(cell->y == 0);
}

TEST_CASE("a negative pixel coordinate is out of bounds") {
    CHECK_FALSE(BoardMapper::pixel_to_cell(-1, 0, 8, 8).has_value());
    CHECK_FALSE(BoardMapper::pixel_to_cell(0, -1, 8, 8).has_value());
}

TEST_CASE("a pixel at or beyond the board's far edge is out of bounds") {
    CHECK_FALSE(BoardMapper::pixel_to_cell(8 * constants::kCellSizePx, 0, 8, 8).has_value());
    CHECK_FALSE(BoardMapper::pixel_to_cell(0, 8 * constants::kCellSizePx, 8, 8).has_value());
}

TEST_CASE("a degenerate 0x0 board has no valid cells") {
    CHECK_FALSE(BoardMapper::pixel_to_cell(0, 0, 0, 0).has_value());
}

}

TEST_SUITE("BoardMapper::cell_to_algebraic") {

TEST_CASE("the top-left cell is the 'a' file, back rank of an 8-row board") {
    CHECK(BoardMapper::cell_to_algebraic(Position{ 0, 0 }, 8) == "a8");
}

TEST_CASE("the bottom-left cell is a1, matching white's home rank") {
    CHECK(BoardMapper::cell_to_algebraic(Position{ 0, 7 }, 8) == "a1");
}

TEST_CASE("the bottom-right cell is h1") {
    CHECK(BoardMapper::cell_to_algebraic(Position{ 7, 7 }, 8) == "h1");
}

TEST_CASE("the top-right cell is h8") {
    CHECK(BoardMapper::cell_to_algebraic(Position{ 7, 0 }, 8) == "h8");
}

}
