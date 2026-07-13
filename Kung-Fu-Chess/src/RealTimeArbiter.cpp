#include "RealTimeArbiter.h"

#include <algorithm>
#include <limits>

#include "Constants.h"
#include "Piece.h"

namespace {

    int abs_diff(int a, int b) {
        return a > b ? a - b : b - a;
    }

    // Every cell a piece passes through from start to dest, inclusive. For a
    // non-straight, non-diagonal offset (e.g. a knight's L-shape), nothing
    // lies in between, so the path is just the two endpoints.
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

    // A mover occupies a path cell from enter_ms; it vacates at exit_ms,
    // except at its destination (the final cell), which it holds
    // indefinitely once arrived - modeled as kNoExit so a still-pending
    // destination cell keeps colliding with anything that reaches it later.
    struct CellWindow {
        long long enter_ms;
        long long exit_ms;
    };

    constexpr long long kNoExit = std::numeric_limits<long long>::max();

    // One occupancy window per cell of `path`, in path order; `scheduled_ms`
    // is the clock time the move was scheduled (the "enter" instant for the
    // start cell, index 0).
    std::vector<CellWindow> occupancy_windows(const std::vector<Position>& path, long long scheduled_ms,
                                               long long move_ms_per_cell) {
        std::vector<CellWindow> windows;
        windows.reserve(path.size());
        for (std::size_t i = 0; i < path.size(); ++i) {
            long long enter_ms = scheduled_ms + static_cast<long long>(i) * move_ms_per_cell;
            bool is_destination = (i + 1 == path.size());
            windows.push_back(CellWindow{ enter_ms, is_destination ? kNoExit : enter_ms + move_ms_per_cell });
        }
        return windows;
    }

    // Closed-interval overlap: touching endpoints count, so a head-on swap
    // that crosses between grid cells over an odd distance is still caught.
    bool windows_overlap(const CellWindow& a, const CellWindow& b) {
        return std::max(a.enter_ms, b.enter_ms) <= std::min(a.exit_ms, b.exit_ms);
    }

} // namespace

RealTimeArbiter::RealTimeArbiter(Board& board, long long move_ms_per_cell)
    : board_(board), move_ms_per_cell_(move_ms_per_cell) {
}

bool RealTimeArbiter::is_moving(int x, int y) const {
    for (const PendingMove& move : pending_moves_) {
        if (move.start == Position{ x, y }) {
            return true;
        }
    }
    return false;
}

const RealTimeArbiter::AirbornePiece* RealTimeArbiter::airborne_at(int x, int y) const {
    for (const AirbornePiece& airborne : airborne_) {
        if (airborne.cell == Position{ x, y }) {
            return &airborne;
        }
    }
    return nullptr;
}

void RealTimeArbiter::drop_airborne_at(int x, int y) {
    std::vector<AirbornePiece> kept;
    for (const AirbornePiece& airborne : airborne_) {
        if (airborne.cell.x != x || airborne.cell.y != y) {
            kept.push_back(airborne);
        }
    }
    airborne_ = std::move(kept);
}

// Travel time is proportional to Chebyshev distance (diagonals cost the
// same as straight moves).
long long RealTimeArbiter::arrival_time_for(int start_x, int start_y, int dest_x, int dest_y) const {
    int distance_cells = std::max(abs_diff(start_x, dest_x), abs_diff(start_y, dest_y));
    return clock_ms_ + static_cast<long long>(distance_cells) * move_ms_per_cell_;
}

bool RealTimeArbiter::captures_enemy_king(const PendingMove& move) const {
    std::optional<Cell> target = board_.get_at(move.dest.x, move.dest.y);
    return target.has_value() && target->type == PieceType::K && target->color != move.piece.color;
}

// Farthest row is row 0 for white (which moves up), the last row for black.
bool RealTimeArbiter::is_pawn_promotion(const PendingMove& move) const {
    if (move.piece.type != PieceType::P) {
        return false;
    }
    int last_row = (move.piece.color == Color::w) ? 0 : board_.get_height() - 1;
    return move.dest.y == last_row;
}

bool RealTimeArbiter::settle_arrived_moves() {
    // Fast path: nothing to settle this tick, so skip rebuilding either vector.
    bool any_move_arrived = false;
    for (const PendingMove& move : pending_moves_) {
        if (move.arrival_ms <= clock_ms_) {
            any_move_arrived = true;
            break;
        }
    }
    bool any_jump_landed = false;
    for (const AirbornePiece& airborne : airborne_) {
        if (airborne.land_ms <= clock_ms_) {
            any_jump_landed = true;
            break;
        }
    }
    if (!any_move_arrived && !any_jump_landed) {
        return false;
    }

    bool king_captured = false;
    std::vector<PendingMove> still_pending;

    for (const PendingMove& move : pending_moves_) {
        if (move.arrival_ms > clock_ms_) {
            still_pending.push_back(move);
            continue;
        }

        // An airborne enemy on the destination captures the arriving piece
        // instead of being captured: clear the mover's origin and skip
        // placing it; the jumper stays untouched since it never left its cell.
        const AirbornePiece* guard = airborne_at(move.dest.x, move.dest.y);
        if (guard != nullptr && guard->piece.color != move.piece.color
            && move.arrival_ms <= guard->land_ms) {
            board_.clear_at(move.start.x, move.start.y);
            if (move.piece.type == PieceType::K) {
                king_captured = true;
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
        // A regular move that lands puts the piece on cooldown; jumps don't.
        piece.cooldown_end_ms = clock_ms_ + constants::kCooldownMs;
        // Drop any stale airborne record for the destination piece we're
        // about to overwrite.
        drop_airborne_at(move.dest.x, move.dest.y);
        board_.place_at(move.dest.x, move.dest.y, piece);
        board_.clear_at(move.start.x, move.start.y);
    }

    pending_moves_ = std::move(still_pending);

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
        clock_ms_,
        next_sequence_++,
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
    // Run both unconditionally: a collision loser's King and a settled
    // arrival's King capture are independent ways the game can end this tick.
    bool king_lost_to_collision = resolve_collisions();
    bool king_captured_while_settling = settle_arrived_moves();
    return king_lost_to_collision || king_captured_while_settling;
}

// Neither move collides with anything if either piece can pass through
// units (only the knight, today). Otherwise, scans the winning mover's own
// path in order and returns the first cell it shares with the other
// mover's path whose occupancy windows overlap and are already due.
std::optional<Position> RealTimeArbiter::due_collision_cell(const PendingMove& a, const PendingMove& b) const {
    const Piece* piece_a = PieceFactory::get_piece(a.piece.type);
    const Piece* piece_b = PieceFactory::get_piece(b.piece.type);
    if ((piece_a != nullptr && piece_a->can_pass_through_units())
        || (piece_b != nullptr && piece_b->can_pass_through_units())) {
        return std::nullopt;
    }

    const PendingMove& winner = (a.sequence < b.sequence) ? a : b;
    const PendingMove& other = (a.sequence < b.sequence) ? b : a;

    std::vector<Position> winner_path = path_cells(winner.start.x, winner.start.y, winner.dest.x, winner.dest.y);
    std::vector<Position> other_path = path_cells(other.start.x, other.start.y, other.dest.x, other.dest.y);
    std::vector<CellWindow> winner_windows =
        occupancy_windows(winner_path, winner.scheduled_ms, move_ms_per_cell_);
    std::vector<CellWindow> other_windows =
        occupancy_windows(other_path, other.scheduled_ms, move_ms_per_cell_);

    for (std::size_t wi = 0; wi < winner_path.size(); ++wi) {
        for (std::size_t oi = 0; oi < other_path.size(); ++oi) {
            if (!(winner_path[wi] == other_path[oi])) {
                continue;
            }
            bool is_due = std::max(winner_windows[wi].enter_ms, other_windows[oi].enter_ms) <= clock_ms_;
            if (windows_overlap(winner_windows[wi], other_windows[oi]) && is_due) {
                return winner_path[wi];
            }
        }
    }
    return std::nullopt;
}

void RealTimeArbiter::apply_collision(std::size_t winner_index, std::size_t loser_index, Position collision_cell) {
    PendingMove& winner = pending_moves_[winner_index];
    int distance_cells =
        std::max(abs_diff(winner.start.x, collision_cell.x), abs_diff(winner.start.y, collision_cell.y));
    winner.dest = collision_cell;
    winner.arrival_ms = winner.scheduled_ms + static_cast<long long>(distance_cells) * move_ms_per_cell_;

    board_.clear_at(pending_moves_[loser_index].start.x, pending_moves_[loser_index].start.y);
    pending_moves_.erase(pending_moves_.begin() + static_cast<std::ptrdiff_t>(loser_index));
}

bool RealTimeArbiter::resolve_next_collision(bool& king_captured) {
    for (std::size_t i = 0; i < pending_moves_.size(); ++i) {
        for (std::size_t j = i + 1; j < pending_moves_.size(); ++j) {
            std::optional<Position> collision_cell = due_collision_cell(pending_moves_[i], pending_moves_[j]);
            if (!collision_cell.has_value()) {
                continue;
            }
            std::size_t winner_index = (pending_moves_[i].sequence < pending_moves_[j].sequence) ? i : j;
            std::size_t loser_index = (winner_index == i) ? j : i;
            // A King lost as a collision loser ends the game, same as a
            // normal capture; check before apply_collision erases it.
            if (pending_moves_[loser_index].piece.type == PieceType::K) {
                king_captured = true;
            }
            apply_collision(winner_index, loser_index, *collision_cell);
            return true;
        }
    }
    return false;
}

bool RealTimeArbiter::resolve_collisions() {
    // Removing a loser can expose another due collision (e.g. a third piece
    // sharing the loser's path), so keep scanning until none remain.
    bool king_captured = false;
    while (resolve_next_collision(king_captured)) {
    }
    return king_captured;
}
