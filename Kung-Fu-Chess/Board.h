#pragma once

#include <vector>
#include <optional>

#include "Types.h"

class Board {
public:
    Board(int width, int height);

    int get_width() const { return width; }
    int get_height() const { return height; }

    std::optional<Cell> get_at(int x, int y) const;
    void place_at(int x, int y, const Cell& cell);
    void clear_at(int x, int y);

private:
    int width;
    int height;
    std::vector<std::optional<Cell>> cells;

    int index(int x, int y) const;
    void check_bounds(int x, int y) const;
};
