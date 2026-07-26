#pragma once

#include <optional>
#include <string>

#include "Position.h"

class BoardMapper {
public:
    static std::optional<Position> pixel_to_cell(int pixel_x, int pixel_y, int board_width, int board_height);

    // e.g. {0, 0} on an 8-row board -> "a8" (rank 1 is the last row, matching Parser's orientation).
    static std::string cell_to_algebraic(Position cell, int board_height);
};
