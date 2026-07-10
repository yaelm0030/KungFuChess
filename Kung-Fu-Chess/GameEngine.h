#pragma once

#include <iostream>
#include <optional>

#include "Board.h"

struct Position {
    int x;
    int y;
};

class GameEngine {
public:
    explicit GameEngine(Board board);

    // Converts pixel coordinates to a board cell and applies the click rules:
    // selects a piece, replaces the selection with another friendly piece,
    // or sends a move request to the selected cell. Clicks outside the board
    // and clicks on an empty cell with nothing selected are ignored.
    void click(int pixel_x, int pixel_y);

    // Advances the game clock. Moves settle instantly for now, so this only
    // affects clock_ms() until timed moves are introduced.
    void wait(int milliseconds);

    // Prints the current settled board state.
    void print(std::ostream& out = std::cout) const;

    long long clock_ms() const { return clock_ms_; }
    bool has_selection() const { return selected_.has_value(); }
    std::optional<Position> selected() const { return selected_; }

private:
    static constexpr int kCellSizePx = 100;

    Board board_;
    std::optional<Position> selected_;
    long long clock_ms_ = 0;

    std::optional<Position> pixel_to_cell(int pixel_x, int pixel_y) const;
};
