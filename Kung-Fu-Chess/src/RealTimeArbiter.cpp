#include "RealTimeArbiter.h"

#include <algorithm>

namespace {

    int abs_diff(int a, int b) {
        return a > b ? a - b : b - a;
    }

    // Every cell a piece passes through moving from start to dest, inclusive
    // of both endpoints. For a straight line or diagonal, that's the whole
    // line; for any other offset (e.g. a knight's L-shape), the piece
    // doesn't pass through anything in between, so the path is just the two
    // endpoints.
    std::vector<Position> path_cells(int start_x, int start_y, int dest_x, int dest_y) {
        int dx = abs_diff(start_x, dest_x);
        int dy = abs_diff(start_y, dest_y);

        if (dx != 0 && dy != 0 && dx != dy) {
            return { Position{ start_x, start_y }, Position{ dest_x, dest_y } };
        }

        int step_x = (dest_x > start_x) - (dest_x < start_x);
        int step_y = (dest_y > start_y) - (dest_y < start_y);

        std::vector<Position> cells;
        int x = start_x;
        int y = start_y;
        cells.push_back(Position{ x, y });
        while (x != dest_x || y != dest_y) {
            x += step_x;
            y += step_y;
            cells.push_back(Position{ x, y });
        }
        return cells;
    }

} // namespace

RealTimeArbiter::RealTimeArbiter(Board& board, long long move_ms_per_cell)
    : board_(board), move_ms_per_cell_(move_ms_per_cell) {
}

// True if the piece at (x, y) has a pending move that hasn't arrived yet.
bool RealTimeArbiter::is_moving(int x, int y) const {
    for (const PendingMove& move : pending_moves_) {
        if (move.start.x == x && move.start.y == y) {
            return true;
        }
    }
    return false;
}

// The airborne (mid-jump) piece at (x, y), or nullptr if none.
const RealTimeArbiter::AirbornePiece* RealTimeArbiter::airborne_at(int x, int y) const {
    for (const AirbornePiece& airborne : airborne_) {
        if (airborne.cell.x == x && airborne.cell.y == y) {
            return &airborne;
        }
    }
    return nullptr;
}

// Drops any airborne record on (x, y), keeping airborne_ in sync with the board.
void RealTimeArbiter::drop_airborne_at(int x, int y) {
    std::vector<AirbornePiece> kept;
    for (const AirbornePiece& airborne : airborne_) {
        if (airborne.cell.x != x || airborne.cell.y != y) {
            kept.push_back(airborne);
        }
    }
    airborne_ = std::move(kept);
}

// True if a move between the two cells would share a cell with the route of
// any move already in flight.
bool RealTimeArbiter::conflicts_with_pending_move(int start_x, int start_y, int dest_x, int dest_y) const {
    std::vector<Position> new_path = path_cells(start_x, start_y, dest_x, dest_y);
    for (const PendingMove& move : pending_moves_) {
        std::vector<Position> existing_path = path_cells(move.start.x, move.start.y, move.dest.x, move.dest.y);
        for (const Position& a : new_path) {
            for (const Position& b : existing_path) {
                if (a.x == b.x && a.y == b.y) {
                    return true;
                }
            }
        }
    }
    return false;
}

// Computes when a move between the two cells will arrive, based on travel
// time per cell of (Chebyshev) distance.
long long RealTimeArbiter::arrival_time_for(int start_x, int start_y, int dest_x, int dest_y) const {
    int distance_cells = std::max(abs_diff(start_x, dest_x), abs_diff(start_y, dest_y));
    return clock_ms_ + static_cast<long long>(distance_cells) * move_ms_per_cell_;
}

// True if `move` lands on a cell occupied by an enemy king.
bool RealTimeArbiter::captures_enemy_king(const PendingMove& move) const {
    std::optional<Cell> target = board_.get_at(move.dest.x, move.dest.y);
    return target.has_value() && target->type == PieceType::K && target->color != move.piece.color;
}

// True if `move`'s piece is a pawn landing on the farthest row from its own
// side: row 0 for white (which moves up), the last row for black.
bool RealTimeArbiter::is_pawn_promotion(const PendingMove& move) const {
    if (move.piece.type != PieceType::P) {
        return false;
    }
    int last_row = (move.piece.color == Color::w) ? 0 : board_.get_height() - 1;
    return move.dest.y == last_row;
}

// Applies every pending move whose arrival time has passed, and keeps the
// rest queued. Returns true if an enemy king was captured while settling.
bool RealTimeArbiter::settle_arrived_moves() {
    bool king_captured = false;
    std::vector<PendingMove> still_pending;

    for (const PendingMove& move : pending_moves_) {
        if (move.arrival_ms > clock_ms_) {
            still_pending.push_back(move);
            continue;
        }

        // If an enemy piece is jumping on the destination and is still airborne
        // when this move arrives (arrival at or before its landing), the
        // airborne piece captures the arriving enemy. This is resolved
        // explicitly: the arriving enemy is removed (its origin cleared) and its
        // move is consumed here; the jumper is deliberately left untouched on
        // board_ (it never left its cell), so no placement is needed.
        const AirbornePiece* guard = airborne_at(move.dest.x, move.dest.y);
        if (guard != nullptr && guard->piece.color != move.piece.color
            && move.arrival_ms <= guard->land_ms) {
            board_.clear_at(move.start.x, move.start.y);
            if (move.piece.type == PieceType::K) {
                king_captured = true; // the arriving enemy king was destroyed
            }
            continue;
        }

        if (captures_enemy_king(move)) {
            king_captured = true;
        }
        Cell piece = move.piece;
        if (is_pawn_promotion(move)) {
            piece.type = PieceType::Q;
        }
        // The destination piece (if any) is captured. If it had an airborne
        // record whose window has already ended, drop it so airborne_ never
        // keeps a stale entry for the piece we are about to overwrite.
        drop_airborne_at(move.dest.x, move.dest.y);
        board_.place_at(move.dest.x, move.dest.y, piece);
        board_.clear_at(move.start.x, move.start.y);
    }

    pending_moves_ = std::move(still_pending);

    // Land any jumps whose window has ended; the piece is already on the board,
    // so landing is just dropping the airborne status.
    std::vector<AirbornePiece> still_airborne;
    for (const AirbornePiece& airborne : airborne_) {
        if (airborne.land_ms > clock_ms_) {
            still_airborne.push_back(airborne);
        }
    }
    airborne_ = std::move(still_airborne);

    return king_captured;
}

void RealTimeArbiter::schedule_move(Position start, Position dest, Cell piece) {
    pending_moves_.push_back(PendingMove{
        start,
        dest,
        piece,
        arrival_time_for(start.x, start.y, dest.x, dest.y),
    });
}

void RealTimeArbiter::start_jump(Position cell, Cell piece, long long jump_duration_ms) {
    airborne_.push_back(AirbornePiece{ cell, piece, clock_ms_ + jump_duration_ms });
}

bool RealTimeArbiter::advance(int milliseconds) {
    if (milliseconds <= 0) {
        return false;
    }
    clock_ms_ += milliseconds;
    return settle_arrived_moves();
}
