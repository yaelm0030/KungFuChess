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
    bool game_over() const { return game_over_; }

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
    bool game_over_ = false;

    std::optional<Position> pixel_to_cell(int pixel_x, int pixel_y) const;

    // True if the piece at (x, y) is currently mid-route to a destination
    // (i.e. has a pending move that hasn't arrived yet). A moving piece
    // cannot be selected, so it cannot be redirected.
    bool is_moving(int x, int y) const;

    // True if a move from (start_x, start_y) to (dest_x, dest_y) would pass
    // through any cell already on the route of a pending move. Whichever
    // move was scheduled first keeps its claim on the route; a later,
    // colliding move is rejected outright.
    bool conflicts_with_pending_move(int start_x, int start_y, int dest_x, int dest_y) const;

    long long arrival_time_for(int start_x, int start_y, int dest_x, int dest_y) const;
    void settle_arrived_moves();

    // True if `move` arrives on a cell occupied by an enemy king, i.e. this
    // move captures the king and ends the game.
    bool captures_enemy_king(const PendingMove& move) const;

    // True if `move`'s piece is a pawn arriving at the farthest row from its
    // own side, i.e. this move earns a promotion to queen.
    bool is_pawn_promotion(const PendingMove& move) const;

    // Handles a click while a piece is already selected: reselects on a
    // click on another selectable friendly piece, otherwise attempts to
    // move the selection to `cell`. Returns false only when the selected
    // cell turned out to be stale (its piece is gone), in which case the
    // selection is cleared and the click should be treated as a fresh one.
    bool handle_click_with_selection(Position cell, std::optional<Cell> clicked_piece, bool clicked_cell_is_selectable);

    // Attempts to move the currently selected piece to `cell`. Does nothing
    // if the move doesn't match the piece's shape (or its path is blocked),
    // or if its route collides with a move already in flight. On success,
    // queues a pending move and clears the selection.
    void try_schedule_move(Position cell, Cell selected_piece);
};
