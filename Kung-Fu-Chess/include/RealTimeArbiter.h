#pragma once

#include <optional>
#include <vector>

#include "Board.h"
#include "Position.h"

// SRP: owns real-time move/jump scheduling and the simulated clock over a
// bound Board, separate from GameEngine's move-legality concerns. Reports
// king captures back to the caller rather than deciding game-over itself.
class RealTimeArbiter {
public:
    RealTimeArbiter(Board& board, long long move_ms_per_cell);

    // True if the piece at (x, y) has a pending move that hasn't arrived yet.
    bool is_moving(int x, int y) const;

    bool is_airborne(int x, int y) const { return airborne_at(x, y) != nullptr; }

    // Called whenever the piece on (x, y) is replaced, so airborne state
    // never outlives the piece it describes.
    void drop_airborne_at(int x, int y);

    void schedule_move(Position start, Position dest, Cell piece);

    // Guards `cell` for jump_duration_ms: an enemy move that arrives there
    // during the window is captured by the jumper instead of capturing it.
    void start_jump(Position cell, Cell piece, long long jump_duration_ms);

    // Advances the clock and settles arrived moves/jumps. Returns true if
    // an enemy king was captured while settling.
    bool advance(int milliseconds);

    long long clock_ms() const { return clock_ms_; }

    // True if the given move's route would share a cell with a pending
    // move's route. Whichever move was scheduled first keeps its claim; a
    // later, colliding move is rejected outright.
    bool conflicts_with_pending_move(int start_x, int start_y, int dest_x, int dest_y) const;

private:
    struct PendingMove {
        Position start;
        Position dest;
        Cell piece;
        long long arrival_ms;
    };

    // A piece mid-jump. It stays on `cell` on the board for the whole jump
    // (no special handling needed elsewhere); this is just a time-windowed
    // status overlay carrying `land_ms`, kept in sync so it never outlives
    // the piece it describes.
    struct AirbornePiece {
        Position cell;
        Cell piece;
        long long land_ms;
    };

    Board& board_;
    long long move_ms_per_cell_;
    long long clock_ms_ = 0;
    std::vector<PendingMove> pending_moves_;
    std::vector<AirbornePiece> airborne_;

    const AirbornePiece* airborne_at(int x, int y) const;

    long long arrival_time_for(int start_x, int start_y, int dest_x, int dest_y) const;

    bool captures_enemy_king(const PendingMove& move) const;

    // True if `move`'s piece is a pawn reaching the farthest row (promotion).
    bool is_pawn_promotion(const PendingMove& move) const;

    bool settle_arrived_moves();
};
