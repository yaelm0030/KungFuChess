#pragma once

#include <iostream>
#include <optional>
#include <vector>

#include "Board.h"

struct Position {
    int x;
    int y;
};

class GameEngine {
public:
    static constexpr long long kDefaultMoveMsPerCell = 1000;

    explicit GameEngine(Board board, long long move_ms_per_cell = kDefaultMoveMsPerCell);

    // Converts pixel coordinates to a board cell and applies the click rules:
    // selects a piece, replaces the selection with another friendly piece, or
    // sends a move request to the selected cell. A move request is queued as
    // a pending move rather than applied immediately; it lands on the board
    // once wait() advances the clock past its arrival time. Clicks outside
    // the board, on an empty cell with nothing selected, or on a cell whose
    // piece is already moving, are ignored: a moving piece can neither be
    // selected nor redirected mid-route. Once a move settles, the arriving
    // piece is free to move again immediately, with no cooldown.
    void click(int pixel_x, int pixel_y);

    // Advances the game clock and settles any pending moves whose arrival
    // time has now passed.
    void wait(int milliseconds);

    // Prints the current settled board state; pieces with a move in flight
    // still show at their original cell until they arrive.
    void print(std::ostream& out = std::cout) const;

    long long clock_ms() const { return clock_ms_; }
    bool has_selection() const { return selected_.has_value(); }
    std::optional<Position> selected() const { return selected_; }

private:
    static constexpr int kCellSizePx = 100;

    struct PendingMove {
        Position start;
        Position dest;
        Cell piece;
        long long arrival_ms;
    };

    Board board_;
    long long move_ms_per_cell_;
    std::optional<Position> selected_;
    long long clock_ms_ = 0;
    std::vector<PendingMove> pending_moves_;

    std::optional<Position> pixel_to_cell(int pixel_x, int pixel_y) const;

    // True if the piece at (x, y) is currently mid-route to a destination
    // (i.e. has a pending move that hasn't arrived yet). A moving piece
    // cannot be selected, so it cannot be redirected.
    bool is_moving(int x, int y) const;

    bool destination_reserved(int x, int y) const;

    long long arrival_time_for(int start_x, int start_y, int dest_x, int dest_y) const;
    void settle_arrived_moves();
};
