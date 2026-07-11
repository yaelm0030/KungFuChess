#pragma once

#include <optional>
#include <vector>

#include "Board.h"
#include "Position.h"

// Owns everything about real-time movement over a bound Board: pieces
// currently in flight (pending moves) or airborne (mid-jump), and the
// simulated clock that advances and settles them. Applies settled moves
// directly to the bound board. Reports back whether an enemy king was
// captured while settling, rather than tracking a game-over flag itself —
// that decision belongs to the caller.
class RealTimeArbiter {
public:
    RealTimeArbiter(Board& board, long long move_ms_per_cell);

    // True if the piece at (x, y) is currently mid-route to a destination
    // (i.e. has a pending move that hasn't arrived yet). A moving piece
    // cannot be selected, so it cannot be redirected.
    bool is_moving(int x, int y) const;

    // True if the piece at (x, y) is currently mid-jump. An airborne piece
    // can neither be selected nor redirected until it lands.
    bool is_airborne(int x, int y) const { return airborne_at(x, y) != nullptr; }

    // Removes any airborne record on (x, y). Called whenever the piece on
    // that cell is replaced, so airborne state never outlives the piece it
    // describes.
    void drop_airborne_at(int x, int y);

    // Queues a move of `piece` from `start` to `dest`; it lands once the
    // clock reaches its arrival time.
    void schedule_move(Position start, Position dest, Cell piece);

    // Starts a jump for `piece` on `cell`, guarding that cell for
    // `jump_duration_ms`: if an enemy moving piece arrives there while the
    // jump is still active, the jumper captures the arriving enemy instead
    // of being captured.
    void start_jump(Position cell, Cell piece, long long jump_duration_ms);

    // Advances the simulated clock by `milliseconds` (a no-op if not
    // positive) and settles any pending moves/jumps whose time has now
    // passed, applying them directly to the bound board. Returns true if an
    // enemy king was captured while settling (whether by normal arrival or
    // by an airborne guard).
    bool advance(int milliseconds);

    long long clock_ms() const { return clock_ms_; }

    // True if a move from (start_x, start_y) to (dest_x, dest_y) would pass
    // through any cell already on the route of a pending move. Whichever
    // move was scheduled first keeps its claim on the route; a later,
    // colliding move is rejected outright. Called by GameEngine before
    // scheduling a new move, to avoid overlapping real-time commands.
    bool conflicts_with_pending_move(int start_x, int start_y, int dest_x, int dest_y) const;

private:
    struct PendingMove {
        Position start;
        Position dest;
        Cell piece;
        long long arrival_ms;
    };

    // A piece that is mid-jump. Invariants:
    //  - The jumper REMAINS on `cell` on the bound board for the whole jump,
    //    exactly as an in-flight piece stays on its `start` cell, so the
    //    board shows it in place with no special handling.
    //  - `airborne_` is therefore a pure time-windowed status overlay, not a
    //    separate copy of the piece; it is kept in sync with the board (an
    //    entry is dropped the moment its cell is replaced or the window
    //    ends), so it can never outlive the piece it describes.
    //  - It carries only timing (`land_ms`), not any animation/visual state.
    //  - While airborne, the piece cannot be selected, moved, or captured;
    //    it instead captures any enemy that arrives on its cell during the
    //    window.
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

    // True if `move` arrives on a cell occupied by an enemy king, i.e. this
    // move captures the king.
    bool captures_enemy_king(const PendingMove& move) const;

    // True if `move`'s piece is a pawn arriving at the farthest row from its
    // own side, i.e. this move earns a promotion to queen.
    bool is_pawn_promotion(const PendingMove& move) const;

    // Applies every pending move whose arrival time has passed, and keeps
    // the rest queued. Returns true if an enemy king was captured.
    bool settle_arrived_moves();
};
