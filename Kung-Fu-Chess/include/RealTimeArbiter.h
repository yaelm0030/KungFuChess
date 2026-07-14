#pragma once

#include <cstddef>
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

    // Advances the clock, resolves any due mid-movement collisions, and
    // settles arrived moves/jumps. Returns true if an enemy king was
    // captured - either directly while settling, or because a collision
    // removed a King on the losing side.
    bool advance(int milliseconds);

    long long clock_ms() const { return clock_ms_; }

private:
    struct PendingMove {
        Position start;
        Position dest;
        Cell piece;
        long long arrival_ms;
        long long scheduled_ms; // clock_ms_ at the moment this move was scheduled
        long long sequence;     // schedule order; the tiebreaker for "who moved first"
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
    long long next_sequence_ = 0;
    std::vector<PendingMove> pending_moves_;
    std::vector<AirbornePiece> airborne_;

    const AirbornePiece* airborne_at(int x, int y) const;

    long long arrival_time_for(int start_x, int start_y, int dest_x, int dest_y) const;

    bool captures_enemy_king(const PendingMove& move) const;

    // True if `move`'s piece is a pawn reaching the farthest row (promotion).
    bool is_pawn_promotion(const PendingMove& move) const;

    bool settle_arrived_moves();

    // True if `move`'s traversal of its path index `index` (out of
    // `path_length` cells) is a harmless pass-through: the piece can pass
    // through units (only a knight, today) and this isn't its own
    // destination. A pass-through piece can still never land ON a unit, so
    // this is always false at its final path index.
    bool passes_through_at(const PendingMove& move, std::size_t path_length, std::size_t index) const;

    // The first cell (in scan_first's own path order) shared with
    // scan_second's path where both occupancy windows overlap and are
    // already due. For a same-color pair, a cell is skipped (not a
    // collision) if either mover merely passes through it per
    // passes_through_at - a knight can pass over a friendly unit anywhere
    // but its own destination.
    std::optional<Position> first_due_shared_cell(const PendingMove& scan_first, const PendingMove& scan_second,
                                                   bool same_color) const;

    // The cell where `a` and `b` first collide, if any (scanned in the
    // winning - lower-sequence - mover's own path order). A hostile
    // (different-color) pair is entirely exempt if either piece
    // can_pass_through_units(); a same-color pair instead exempts only a
    // pass-through piece's non-destination cells (see passes_through_at).
    std::optional<Position> due_collision_cell(const PendingMove& a, const PendingMove& b) const;

    // Truncates the winner's dest/arrival_ms to `collision_cell` and drops
    // the loser: cleared from the board and removed from pending_moves_.
    void apply_collision(std::size_t winner_index, std::size_t loser_index, Position collision_cell);

    // Friendly (same-color) resolution: the higher-sequence mover at
    // `yielder_index` yields to `other_index`, which is left untouched.
    // Truncates the yielder's dest/arrival_ms to the path cell immediately
    // before their shared collision cell (scanned along the yielder's own
    // path). If that collision cell was already the yielder's very next
    // cell (or its own start), yielding is a no-op: its PendingMove is
    // dropped with no board change and no cooldown stamp, since it never
    // actually moved.
    void apply_friendly_yield(std::size_t yielder_index, std::size_t other_index);

    // Finds one currently-due collision among pending_moves_ and resolves
    // it - hostile pairs via apply_collision, same-color pairs via
    // apply_friendly_yield - setting king_captured to true if a hostile
    // loser was a King (a King lost this way ends the game just like a
    // normal capture; a friendly yield never removes a piece from the
    // board, so it can't trigger this). Returns true if one was resolved
    // (pending_moves_ changed), so the caller can rescan for further
    // collisions exposed by it.
    bool resolve_next_collision(bool& king_captured);

    // Repeatedly resolves collisions until none remain due this tick, before
    // arrivals are settled. Runs ahead of settle_arrived_moves() so a
    // truncated winner still settles normally through that unmodified path.
    // Returns true if any collision's loser was a King.
    bool resolve_collisions();
};
