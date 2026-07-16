#include "BoardMapper.h"

#include "Constants.h"

std::optional<Position> BoardMapper::pixel_to_cell(int pixel_x, int pixel_y, int board_width, int board_height) {
    if (pixel_x < 0 || pixel_y < 0) {
        return std::nullopt;
    }

    int cell_x = pixel_x / constants::kCellSizePx;
    int cell_y = pixel_y / constants::kCellSizePx;

    if (cell_x >= board_width || cell_y >= board_height) {
        return std::nullopt;
    }

    return Position{ cell_x, cell_y };
}

std::string BoardMapper::cell_to_algebraic(Position cell, int board_height) {
    char file = static_cast<char>('a' + cell.x);
    int rank = board_height - cell.y;
    return std::string(1, file) + std::to_string(rank);
}
