#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "Board.h"
#include "Position.h"

class RealTimeArbiter {
public:
    RealTimeArbiter(Board& board, long long move_ms_per_cell);

    bool is_moving(int x, int y) const;
    bool is_airborne(int x, int y) const { return airborne_at(x, y) != nullptr; }

    // Call whenever the piece on (x, y) is replaced, so airborne state never outlives it.
    void drop_airborne_at(int x, int y);

    void schedule_move(Position start, Position dest, Cell piece);

    // An enemy move arriving at `cell` during the jump is captured by the jumper, not the other way round.
    void start_jump(Position cell, Cell piece, long long jump_duration_ms);

    // True if an enemy king was captured, directly or via a collision.
    bool advance(int milliseconds);

    long long clock_ms() const { return clock_ms_; }

private:
    struct PendingMove {
        Position start;
        Position dest;
        Cell piece;
        long long arrival_ms;
        long long scheduled_ms;
        long long sequence; // schedule order; tiebreaker for "who moved first"
    };

    // A piece mid-jump; stays on `cell` on the board the whole time, so nothing
    // else needs special-casing for it.
    struct AirbornePiece {
        Position cell;
        Cell piece;
        long long land_ms;
    };

    enum class CollisionKind { Hostile, Friendly };

    struct MoverWindow {
        const PendingMove& move;
        long long enter_ms;
        long long exit_ms;
        std::size_t index;
        std::size_t path_length;
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

    bool is_pawn_promotion(const PendingMove& move) const;

    // Fast path so a quiet tick can skip rebuilding pending_moves_/airborne_.
    bool has_arrivals_to_settle() const;

    void settle_one_arrived_move(const PendingMove& move, bool& king_captured);
    bool settle_arrived_moves();

    // True if `move` merely passes through path index `index` (out of
    // `path_length`): the piece can pass through units (only a knight) and
    // this isn't its own destination.
    bool passes_through_at(const PendingMove& move, std::size_t path_length, std::size_t index) const;

    bool is_due_collision_at(const MoverWindow& first, const MoverWindow& second, CollisionKind kind) const;

    std::optional<Position> first_due_shared_cell(const PendingMove& scan_first, const PendingMove& scan_second,
                                                   CollisionKind kind) const;

    // True if `a` was scheduled before `b`; the tiebreaker for which of two colliding movers wins.
    bool has_priority(const PendingMove& a, const PendingMove& b) const;

    // The cell where `a` and `b` first collide, if any.
    std::optional<Position> due_collision_cell(const PendingMove& a, const PendingMove& b) const;

    void apply_collision(std::size_t winner_index, std::size_t loser_index, Position collision_cell);

    // The higher-sequence mover at `yielder_index` yields; truncated to the path
    // cell just before `collision_cell`, or dropped outright if that's its own start.
    void apply_friendly_yield(std::size_t yielder_index, Position collision_cell);

    struct DueCollision {
        std::size_t winner_index;
        std::size_t loser_index;
        Position collision_cell;
    };

    std::optional<DueCollision> find_due_collision() const;
    bool resolve_next_collision(bool& king_captured);

    // Runs ahead of settle_arrived_moves() so a truncated winner still settles normally.
    bool resolve_collisions();
};
