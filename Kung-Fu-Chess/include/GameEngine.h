#pragma once

#include <iostream>
#include <optional>

#include "Board.h"
#include "Constants.h"
#include "RealTimeArbiter.h"

class GameEngine {
public:
    static constexpr long long kDefaultMoveMsPerCell = constants::kDefaultMoveMsPerCell;
    static constexpr long long kJumpDurationMs = constants::kJumpDurationMs;

    explicit GameEngine(Board board, long long move_ms_per_cell = kDefaultMoveMsPerCell);

    // Requests to move the piece at `start` onto `dest`. Validates the move
    // against that piece's own movement rule (shape, blockers, and
    // same-color capture, all via RuleEngine/PieceFactory) and rejects it if
    // it would collide with a move already in flight along its route. On
    // success, queues the move with RealTimeArbiter and returns true;
    // otherwise the board is left unchanged and this returns false. Always
    // false once the game is over, if there is no piece at `start`, or if
    // that piece is currently airborne.
    bool request_move(Position start, Position dest);

    // Requests that the piece at `cell` begin a jump in place for
    // kJumpDurationMs. Returns true if the jump started. Always false once
    // the game is over, if there is no piece at `cell`, or if that piece is
    // already moving or already airborne.
    bool request_jump(Position cell);

    // Advances the game clock and settles any pending moves whose arrival
    // time has now passed.
    void wait(int milliseconds);

    // Prints the current settled board state; pieces with a move in flight
    // still show at their original cell until they arrive.
    void print(std::ostream& out = std::cout) const;

    long long clock_ms() const { return arbiter_.clock_ms(); }
    bool game_over() const { return game_over_; }

    int width() const { return board_.get_width(); }
    int height() const { return board_.get_height(); }

    // True if `cell` holds a piece that could currently be selected: it is
    // present, and neither mid-move nor mid-jump.
    bool is_selectable(Position cell) const;

    // The color of the piece at `cell`, or nullopt if the cell is empty.
    std::optional<Color> color_at(Position cell) const;

private:
    Board board_;
    RealTimeArbiter arbiter_;
    bool game_over_ = false;
};
