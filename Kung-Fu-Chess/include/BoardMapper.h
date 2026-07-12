#pragma once

#include <optional>

#include "Position.h"

// Adapter: converts pixel coordinates into logical board cells; knows
// nothing about chess, pieces, or selection state.
class BoardMapper {
public:
    // Returns the cell at (pixel_x, pixel_y), or nullopt if outside the board.
    static std::optional<Position> pixel_to_cell(int pixel_x, int pixel_y, int board_width, int board_height);
};
