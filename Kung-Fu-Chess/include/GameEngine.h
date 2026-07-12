#pragma once

#include <iostream>
#include <optional>

#include "Board.h"
#include "Constants.h"
#include "RealTimeArbiter.h"

// Facade: coordinates Board, RealTimeArbiter, and the Piece/RuleEngine move
// rules behind one simple move/jump/wait/print interface.
class GameEngine {
public:
    static constexpr long long kDefaultMoveMsPerCell = constants::kDefaultMoveMsPerCell;
    static constexpr long long kJumpDurationMs = constants::kJumpDurationMs;

    explicit GameEngine(Board board, long long move_ms_per_cell = kDefaultMoveMsPerCell);

    // Validates the move against the piece's own rule and against any move
    // already in flight on its route, then queues it via RealTimeArbiter.
    // False (board unchanged) if illegal, the game is over, there's no
    // piece at `start`, or it's airborne.
    bool request_move(Position start, Position dest);

    // Starts a jump in place at `cell` for kJumpDurationMs. False if the
    // game is over, there's no piece there, or it's already moving/airborne.
    bool request_jump(Position cell);

    // Advances the game clock and settles any pending moves whose arrival
    // time has now passed.
    void wait(int milliseconds);

    // Prints the settled board; pieces mid-move still show at their origin.
    void print(std::ostream& out = std::cout) const;

    long long clock_ms() const { return arbiter_.clock_ms(); }
    bool game_over() const { return game_over_; }

    int width() const { return board_.get_width(); }
    int height() const { return board_.get_height(); }

    // True if `cell` holds a piece that can be selected: present, neither
    // mid-move nor mid-jump, and the game is not already over.
    bool is_selectable(Position cell) const;

    std::optional<Color> color_at(Position cell) const;

private:
    Board board_;
    RealTimeArbiter arbiter_;
    bool game_over_ = false;
};
