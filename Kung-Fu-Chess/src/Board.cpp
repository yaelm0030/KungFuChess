#include "Board.h"

#include <stdexcept>

Board::Board(int width, int height)
    : width(width), height(height), cells(static_cast<size_t>(width) * static_cast<size_t>(height)) {
}

int Board::index(int x, int y) const {
    return y * width + x;
}

void Board::check_bounds(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        throw std::out_of_range("Board coordinates out of range");
    }
}

std::optional<Cell> Board::get_at(int x, int y) const {
    check_bounds(x, y);
    return cells[index(x, y)];
}

void Board::place_at(int x, int y, const Cell& cell) {
    check_bounds(x, y);
    cells[index(x, y)] = cell;
}

void Board::clear_at(int x, int y) {
    check_bounds(x, y);
    cells[index(x, y)] = std::nullopt;
}
