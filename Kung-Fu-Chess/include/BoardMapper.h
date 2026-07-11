#pragma once

#include <optional>

#include "Position.h"

// Coordinate adapter: converts screen pixel coordinates into logical board
// cells. Knows nothing about chess, pieces, or selection state -- only the
// pixel size of a cell and the board's dimensions.
class BoardMapper {
public:
    // Returns the cell containing (pixel_x, pixel_y), or nullopt if it
    // falls outside a board of the given dimensions.
    static std::optional<Position> pixel_to_cell(int pixel_x, int pixel_y, int board_width, int board_height);
};
