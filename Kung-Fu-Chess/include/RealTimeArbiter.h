#pragma once

#include <vector>
#include <optional>
#include <cstddef>

#include "Board.h"
#include "Piece.h"
#include <Position.h>

class RealTimeArbiter {
public:
    struct PendingMove {
        Position start;
        Position dest;
        Cell piece;
        long long scheduled_ms;
        long long arrival_ms;
        long long sequence;
    };

    struct AirbornePiece {
        Position cell;
        Cell piece;
        long long land_ms;
    };

    struct MoveProgress {
        Position dest;
        long long scheduled_ms;
        long long arrival_ms;
    };

    RealTimeArbiter(Board& board, long long move_ms_per_cell);

    bool is_moving(int x, int y) const;
    std::optional<MoveProgress> move_progress_at(int x, int y) const;
    bool is_airborne(int x, int y) const { return airborne_at(x, y) != nullptr; }
    long long clock_ms() const { return clock_ms_; }

    void drop_airborne_at(int x, int y);
    void schedule_move(Position start, Position dest, Cell piece);
    void start_jump(Position cell, Cell piece, long long jump_duration_ms);

    bool advance(int milliseconds);

private:
    Board& board_;
    long long move_ms_per_cell_;
    long long clock_ms_ = 0;
    long long next_sequence_ = 0;

    std::vector<PendingMove> pending_moves_;
    std::vector<AirbornePiece> airborne_;

    const AirbornePiece* airborne_at(int x, int y) const;
    static long long get_distance(Position a, Position b);
    static std::vector<Position> get_path(Position start, Position dest);

    bool has_arrivals_to_settle() const;
    bool settle_arrived_moves();
    bool captures_king(const PendingMove& move) const;
    bool is_pawn_promotion(const PendingMove& move) const;

    bool resolve_collisions();
    std::optional<Position> check_collision(const PendingMove& a, const PendingMove& b);
    std::optional<Position> check_static_board_collision(const PendingMove& move);
    void apply_hostile_collision(std::size_t winner_idx, std::size_t loser_idx, Position cell);
    void apply_friendly_yield(std::size_t yielder_idx, Position cell);
};