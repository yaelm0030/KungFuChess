#include "RealTimeArbiter.h"

#include <algorithm>
#include <limits>

#include "Constants.h"
#include "Piece.h"

namespace {

    int abs_diff(int a, int b) {
        return a > b ? a - b : b - a;
    }

    // Travel time is proportional to Chebyshev distance (diagonals cost the
    // same as straight moves).
    long long chebyshev_distance(int x1, int y1, int x2, int y2) {
        return std::max(abs_diff(x1, x2), abs_diff(y1, y2));
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

    // A destination cell holds indefinitely once arrived (kNoExit), so a
    // still-pending destination keeps colliding with anything that reaches it later.
    struct CellWindow {
        long long enter_ms;
        long long exit_ms;
    };

    constexpr long long kNoExit = std::numeric_limits<long long>::max();

    // Each cell's enter instant is timed by its actual Chebyshev distance from
    // the path's start, not its list index: a knight's path is just its two
    // endpoints, so its destination is 2+ cells of distance at index 1.
    std::vector<CellWindow> occupancy_windows(const std::vector<Position>& path, long long scheduled_ms,
                                               long long move_ms_per_cell) {
        std::vector<CellWindow> windows;
        windows.reserve(path.size());
        for (std::size_t i = 0; i < path.size(); ++i) {
            long long dist = chebyshev_distance(path[i].x, path[i].y, path.front().x, path.front().y);
            long long enter_ms = scheduled_ms + dist * move_ms_per_cell;
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

long long RealTimeArbiter::arrival_time_for(int start_x, int start_y, int dest_x, int dest_y) const {
    return clock_ms_ + chebyshev_distance(start_x, start_y, dest_x, dest_y) * move_ms_per_cell_;
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

bool RealTimeArbiter::has_arrivals_to_settle() const {
    for (const PendingMove& move : pending_moves_) {
        if (move.arrival_ms <= clock_ms_) {
            return true;
        }
    }
    for (const AirbornePiece& airborne : airborne_) {
        if (airborne.land_ms <= clock_ms_) {
            return true;
        }
    }
    return false;
}

void RealTimeArbiter::settle_one_arrived_move(const PendingMove& move, bool& king_captured) {
    // An airborne enemy guarding the destination captures the arriver instead
    // of being captured by it; the jumper itself stays untouched.
    const AirbornePiece* guard = airborne_at(move.dest.x, move.dest.y);
    if (guard != nullptr && guard->piece.color != move.piece.color && move.arrival_ms <= guard->land_ms) {
        board_.clear_at(move.start.x, move.start.y);
        if (move.piece.type == PieceType::K) {
            king_captured = true;
        }
        return;
    }

    if (captures_enemy_king(move)) {
        king_captured = true;
    }
    Cell piece = move.piece;
    if (is_pawn_promotion(move)) {
        piece.type = PieceType::Q;
    }
    piece.cooldown_end_ms = clock_ms_ + constants::kCooldownMs;
    drop_airborne_at(move.dest.x, move.dest.y);
    board_.place_at(move.dest.x, move.dest.y, piece);
    board_.clear_at(move.start.x, move.start.y);
}

bool RealTimeArbiter::settle_arrived_moves() {
    if (!has_arrivals_to_settle()) {
        return false;
    }

    bool king_captured = false;
    std::vector<PendingMove> still_pending;
    for (const PendingMove& move : pending_moves_) {
        if (move.arrival_ms > clock_ms_) {
            still_pending.push_back(move);
            continue;
        }
        settle_one_arrived_move(move, king_captured);
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

bool RealTimeArbiter::passes_through_at(const PendingMove& move, std::size_t path_length, std::size_t index) const {
    if (index + 1 == path_length) {
        return false; // its own destination: it must land there, never just "pass through"
    }
    const Piece* piece = PieceFactory::get_piece(move.piece.type);
    return piece != nullptr && piece->can_pass_through_units();
}

bool RealTimeArbiter::is_due_collision_at(const MoverWindow& first, const MoverWindow& second,
                                           CollisionKind kind) const {
    bool is_due = std::max(first.enter_ms, second.enter_ms) <= clock_ms_;
    if (!windows_overlap(CellWindow{ first.enter_ms, first.exit_ms }, CellWindow{ second.enter_ms, second.exit_ms })
        || !is_due) {
        return false;
    }
    // Hostile pairs are already exempted wholesale in due_collision_cell.
    if (kind == CollisionKind::Friendly && (passes_through_at(first.move, first.path_length, first.index)
                                             || passes_through_at(second.move, second.path_length, second.index))) {
        return false;
    }
    return true;
}

std::optional<Position> RealTimeArbiter::first_due_shared_cell(const PendingMove& scan_first,
                                                                 const PendingMove& scan_second,
                                                                 CollisionKind kind) const {
    std::vector<Position> first_path = path_cells(scan_first.start.x, scan_first.start.y, scan_first.dest.x,
                                                    scan_first.dest.y);
    std::vector<Position> second_path = path_cells(scan_second.start.x, scan_second.start.y, scan_second.dest.x,
                                                     scan_second.dest.y);
    std::vector<CellWindow> first_windows =
        occupancy_windows(first_path, scan_first.scheduled_ms, move_ms_per_cell_);
    std::vector<CellWindow> second_windows =
        occupancy_windows(second_path, scan_second.scheduled_ms, move_ms_per_cell_);

    for (std::size_t fi = 0; fi < first_path.size(); ++fi) {
        for (std::size_t si = 0; si < second_path.size(); ++si) {
            if (!(first_path[fi] == second_path[si])) {
                continue;
            }
            MoverWindow first{ scan_first, first_windows[fi].enter_ms, first_windows[fi].exit_ms, fi,
                                first_path.size() };
            MoverWindow second{ scan_second, second_windows[si].enter_ms, second_windows[si].exit_ms, si,
                                 second_path.size() };
            if (is_due_collision_at(first, second, kind)) {
                return first_path[fi];
            }
        }
    }
    return std::nullopt;
}

bool RealTimeArbiter::has_priority(const PendingMove& a, const PendingMove& b) const {
    return a.sequence < b.sequence;
}

// A hostile pair is entirely exempt if either piece can_pass_through_units().
std::optional<Position> RealTimeArbiter::due_collision_cell(const PendingMove& a, const PendingMove& b) const {
    CollisionKind kind = (a.piece.color == b.piece.color) ? CollisionKind::Friendly : CollisionKind::Hostile;
    if (kind == CollisionKind::Hostile) {
        const Piece* piece_a = PieceFactory::get_piece(a.piece.type);
        const Piece* piece_b = PieceFactory::get_piece(b.piece.type);
        if ((piece_a != nullptr && piece_a->can_pass_through_units())
            || (piece_b != nullptr && piece_b->can_pass_through_units())) {
            return std::nullopt;
        }
    }

    bool a_has_priority = has_priority(a, b);
    const PendingMove& winner = a_has_priority ? a : b;
    const PendingMove& other = a_has_priority ? b : a;
    // Friendly scans the yielding mover's own path first, since that's whose
    // path apply_friendly_yield will truncate.
    if (kind == CollisionKind::Friendly) {
        return first_due_shared_cell(other, winner, kind);
    }
    return first_due_shared_cell(winner, other, kind);
}

void RealTimeArbiter::apply_collision(std::size_t winner_index, std::size_t loser_index, Position collision_cell) {
    PendingMove& winner = pending_moves_[winner_index];
    long long distance_cells = chebyshev_distance(winner.start.x, winner.start.y, collision_cell.x, collision_cell.y);
    winner.dest = collision_cell;
    winner.arrival_ms = winner.scheduled_ms + distance_cells * move_ms_per_cell_;

    board_.clear_at(pending_moves_[loser_index].start.x, pending_moves_[loser_index].start.y);
    pending_moves_.erase(pending_moves_.begin() + static_cast<std::ptrdiff_t>(loser_index));
}

void RealTimeArbiter::apply_friendly_yield(std::size_t yielder_index, Position collision_cell) {
    PendingMove& yielder = pending_moves_[yielder_index];

    std::vector<Position> yielder_path = path_cells(yielder.start.x, yielder.start.y, yielder.dest.x, yielder.dest.y);
    std::size_t collision_index = 0;
    while (collision_index < yielder_path.size() && !(yielder_path[collision_index] == collision_cell)) {
        ++collision_index;
    }

    if (collision_index <= 1) {
        // Stopping one cell short is its own start, i.e. it never actually moved.
        pending_moves_.erase(pending_moves_.begin() + static_cast<std::ptrdiff_t>(yielder_index));
        return;
    }

    Position stop_cell = yielder_path[collision_index - 1];
    long long distance_cells = chebyshev_distance(yielder.start.x, yielder.start.y, stop_cell.x, stop_cell.y);
    yielder.dest = stop_cell;
    yielder.arrival_ms = yielder.scheduled_ms + distance_cells * move_ms_per_cell_;
}

std::optional<RealTimeArbiter::DueCollision> RealTimeArbiter::find_due_collision() const {
    for (std::size_t i = 0; i < pending_moves_.size(); ++i) {
        for (std::size_t j = i + 1; j < pending_moves_.size(); ++j) {
            std::optional<Position> collision_cell = due_collision_cell(pending_moves_[i], pending_moves_[j]);
            if (!collision_cell.has_value()) {
                continue;
            }
            bool i_has_priority = has_priority(pending_moves_[i], pending_moves_[j]);
            return DueCollision{ i_has_priority ? i : j, i_has_priority ? j : i, *collision_cell };
        }
    }
    return std::nullopt;
}

bool RealTimeArbiter::resolve_next_collision(bool& king_captured) {
    std::optional<DueCollision> due = find_due_collision();
    if (!due.has_value()) {
        return false;
    }

    if (pending_moves_[due->winner_index].piece.color == pending_moves_[due->loser_index].piece.color) {
        apply_friendly_yield(due->loser_index, due->collision_cell);
    } else {
        // Check before apply_collision erases the loser.
        if (pending_moves_[due->loser_index].piece.type == PieceType::K) {
            king_captured = true;
        }
        apply_collision(due->winner_index, due->loser_index, due->collision_cell);
    }
    return true;
}

bool RealTimeArbiter::resolve_collisions() {
    // Removing a loser can expose another due collision, so keep scanning until none remain.
    bool king_captured = false;
    while (resolve_next_collision(king_captured)) {
    }
    return king_captured;
}
