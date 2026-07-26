#pragma once

#include <iostream>
#include <optional>

#include "Board.h"
#include "Constants.h"
#include "GameSnapshot.h"
#include "RealTimeArbiter.h"

class GameEngine {
public:
    static constexpr long long kCooldownMs = constants::kCooldownMs;
    static constexpr long long kDefaultMoveMsPerCell = constants::kDefaultMoveMsPerCell;
    static constexpr long long kJumpDurationMs = constants::kJumpDurationMs;

    explicit GameEngine(Board board, long long move_ms_per_cell = kDefaultMoveMsPerCell);

    // An out-of-range start/dest propagates Board's std::out_of_range, unless the game is already over.
    bool request_move(Position start, Position dest);
    bool request_jump(Position cell);

    void wait(int milliseconds);

    // Pieces mid-move still show at their origin.
    void print(std::ostream& out) const;

    long long clock_ms() const { return arbiter_.clock_ms(); }
    bool game_over() const { return game_over_; }

    int width() const { return board_.get_width(); }
    int height() const { return board_.get_height(); }

    bool is_selectable(Position cell) const;

    std::optional<Color> color_at(Position cell) const;

    // Per-piece type/color/position/state snapshot, plus game-over. Mid-move pieces report their origin cell.
    GameSnapshot snapshot() const;

    // Every move successfully scheduled via request_move, in order. Jumps aren't recorded.
    const std::vector<MoveRecord>& move_history() const { return move_history_; }

private:
    Board board_;
    RealTimeArbiter arbiter_;
    bool game_over_ = false;
    std::vector<MoveRecord> move_history_;
};
