#include "RealTimeArbiter.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "Constants.h"

RealTimeArbiter::RealTimeArbiter(Board& board, long long move_ms_per_cell)
    : board_(board), move_ms_per_cell_(move_ms_per_cell) {
}

bool RealTimeArbiter::is_moving(int x, int y) const {
    return std::any_of(pending_moves_.begin(), pending_moves_.end(),
                        [&](const PendingMove& move) { return move.start == Position{ x, y }; });
}

std::optional<RealTimeArbiter::MoveProgress> RealTimeArbiter::move_progress_at(int x, int y) const {
    auto it = std::find_if(pending_moves_.begin(), pending_moves_.end(),
                            [&](const PendingMove& move) { return move.start == Position{ x, y }; });
    if (it == pending_moves_.end()) return std::nullopt;
    return MoveProgress{ it->dest, it->scheduled_ms, it->arrival_ms };
}

const RealTimeArbiter::AirbornePiece* RealTimeArbiter::airborne_at(int x, int y) const {
    auto it = std::find_if(airborne_.begin(), airborne_.end(),
                            [&](const AirbornePiece& airborne) { return airborne.cell == Position{ x, y }; });
    return it == airborne_.end() ? nullptr : &*it;
}

void RealTimeArbiter::drop_airborne_at(int x, int y) {
    std::erase_if(airborne_, [&](const AirbornePiece& airborne) { return airborne.cell == Position{ x, y }; });
}

void RealTimeArbiter::schedule_move(Position start, Position dest, Cell piece) {
    long long dist = get_distance(start, dest);
    long long arrival = clock_ms_ + dist * move_ms_per_cell_;
    pending_moves_.push_back(PendingMove{ start, dest, piece, clock_ms_, arrival, next_sequence_++ });
}

void RealTimeArbiter::start_jump(Position cell, Cell piece, long long jump_duration_ms) {
    airborne_.push_back(AirbornePiece{ cell, piece, clock_ms_ + jump_duration_ms });
}

bool RealTimeArbiter::advance(int milliseconds) {
    if (milliseconds <= 0) return false;
    clock_ms_ += milliseconds;

    bool king_lost_to_collision = resolve_collisions();
    bool king_captured_while_settling = settle_arrived_moves();

    return king_lost_to_collision || king_captured_while_settling;
}

long long RealTimeArbiter::get_distance(Position a, Position b) {
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

std::vector<Position> RealTimeArbiter::get_path(Position start, Position dest) {
    int dx = std::abs(start.x - dest.x);
    int dy = std::abs(start.y - dest.y);

    if (dx != 0 && dy != 0 && dx != dy) {
        return { start, dest };
    }

    std::vector<Position> path;
    int step_x = (dest.x > start.x) - (dest.x < start.x);
    int step_y = (dest.y > start.y) - (dest.y < start.y);

    Position curr = start;
    path.push_back(curr);
    while (curr.x != dest.x || curr.y != dest.y) {
        curr.x += step_x;
        curr.y += step_y;
        path.push_back(curr);
    }
    return path;
}

bool RealTimeArbiter::has_arrivals_to_settle() const {
    bool move_due =
        std::any_of(pending_moves_.begin(), pending_moves_.end(), [&](const PendingMove& move) { return move.arrival_ms <= clock_ms_; });
    bool landing_due =
        std::any_of(airborne_.begin(), airborne_.end(), [&](const AirbornePiece& airborne) { return airborne.land_ms <= clock_ms_; });
    return move_due || landing_due;
}

bool RealTimeArbiter::settle_arrived_moves() {
    if (!has_arrivals_to_settle()) return false;

    bool king_captured = false;
    std::vector<PendingMove> still_pending;

    for (const auto& move : pending_moves_) {
        if (move.arrival_ms > clock_ms_) {
            still_pending.push_back(move);
            continue;
        }

        const AirbornePiece* guard = airborne_at(move.dest.x, move.dest.y);
        if (guard != nullptr && guard->piece.color != move.piece.color && move.arrival_ms <= guard->land_ms) {
            captured_pieces_.push_back(move.piece);
            board_.clear_at(move.start.x, move.start.y);
            if (move.piece.type == PieceType::K) king_captured = true;
            continue;
        }

        if (captures_king(move)) king_captured = true;
        std::optional<Cell> captured_at_dest = board_.get_at(move.dest.x, move.dest.y);
        if (captured_at_dest.has_value()) captured_pieces_.push_back(*captured_at_dest);

        Cell piece = move.piece;
        if (is_pawn_promotion(move)) piece.type = PieceType::Q;
        piece.cooldown_end_ms = clock_ms_ + constants::kCooldownMs;

        drop_airborne_at(move.dest.x, move.dest.y);
        board_.place_at(move.dest.x, move.dest.y, piece);
        board_.clear_at(move.start.x, move.start.y);
    }
    pending_moves_ = std::move(still_pending);

    std::erase_if(airborne_, [&](const AirbornePiece& airborne) { return airborne.land_ms <= clock_ms_; });

    return king_captured;
}

bool RealTimeArbiter::resolve_collisions() {
    bool king_captured = false;
    bool found_collision = true;

    while (found_collision) {
        found_collision = false;

        for (std::size_t i = 0; i < pending_moves_.size() && !found_collision; ++i) {
            auto blocked_cell = check_static_board_collision(pending_moves_[i]);
            if (blocked_cell.has_value()) {
                apply_yield(i, *blocked_cell);
                found_collision = true;
            }
        }

        for (std::size_t i = 0; i < pending_moves_.size() && !found_collision; ++i) {
            for (std::size_t j = i + 1; j < pending_moves_.size(); ++j) {
                auto collision_cell = check_collision(pending_moves_[i], pending_moves_[j]);
                if (!collision_cell.has_value()) continue;

                found_collision = true;

                bool i_has_priority = pending_moves_[i].sequence < pending_moves_[j].sequence;
                std::size_t winner_idx = i_has_priority ? i : j;
                std::size_t loser_idx = i_has_priority ? j : i;

                if (pending_moves_[i].piece.color == pending_moves_[j].piece.color) {
                    apply_yield(loser_idx, *collision_cell);
                }
                else {
                    if (pending_moves_[loser_idx].piece.type == PieceType::K) {
                        king_captured = true;
                    }
                    apply_hostile_collision(winner_idx, loser_idx, *collision_cell);
                }
                break;
            }
        }
    }
    return king_captured;
}

std::optional<Position> RealTimeArbiter::check_collision(const PendingMove& a, const PendingMove& b) {
    bool is_hostile = (a.piece.color != b.piece.color);

    if (is_hostile) {
        if (PieceFactory::get_piece(a.piece.type).can_pass_through_units() ||
            PieceFactory::get_piece(b.piece.type).can_pass_through_units()) {
            return std::nullopt;
        }
    }

    auto path_a = get_path(a.start, a.dest);
    auto path_b = get_path(b.start, b.dest);

    for (std::size_t ia = 0; ia < path_a.size(); ++ia) {
        if (!is_hostile && ia + 1 < path_a.size() && PieceFactory::get_piece(a.piece.type).can_pass_through_units()) {
            continue;
        }

        long long enter_a = a.scheduled_ms + get_distance(a.start, path_a[ia]) * move_ms_per_cell_;
        long long exit_a = (ia + 1 == path_a.size()) ? std::numeric_limits<long long>::max() : enter_a + move_ms_per_cell_;

        for (std::size_t ib = 0; ib < path_b.size(); ++ib) {
            if (!(path_a[ia] == path_b[ib])) continue;

            if (!is_hostile && ib + 1 < path_b.size() && PieceFactory::get_piece(b.piece.type).can_pass_through_units()) {
                continue;
            }

            long long enter_b = b.scheduled_ms + get_distance(b.start, path_b[ib]) * move_ms_per_cell_;
            long long exit_b = (ib + 1 == path_b.size()) ? std::numeric_limits<long long>::max() : enter_b + move_ms_per_cell_;

            bool overlap = std::max(enter_a, enter_b) <= std::min(exit_a, exit_b);
            bool is_due = std::max(enter_a, enter_b) <= clock_ms_;

            if (overlap && is_due) {
                return path_a[ia];
            }
        }
    }
    return std::nullopt;
}

std::optional<Position> RealTimeArbiter::check_static_board_collision(const PendingMove& move) {
    if (PieceFactory::get_piece(move.piece.type).can_pass_through_units()) {
        return std::nullopt;
    }

    auto path = get_path(move.start, move.dest);

    for (std::size_t i = 1; i < path.size(); ++i) {
        long long enter_time = move.scheduled_ms + get_distance(move.start, path[i]) * move_ms_per_cell_;

        if (enter_time > clock_ms_) break;

        auto piece_on_board = board_.get_at(path[i].x, path[i].y);

        if (piece_on_board.has_value() && !is_moving(path[i].x, path[i].y)) {
            if (i == path.size() - 1 && piece_on_board->color != move.piece.color) {
                continue;
            }
            return path[i];
        }
    }
    return std::nullopt;
}

void RealTimeArbiter::apply_hostile_collision(std::size_t winner_idx, std::size_t loser_idx, Position cell) {
    auto& winner = pending_moves_[winner_idx];
    winner.dest = cell;
    winner.arrival_ms = winner.scheduled_ms + get_distance(winner.start, cell) * move_ms_per_cell_;

    captured_pieces_.push_back(pending_moves_[loser_idx].piece);
    board_.clear_at(pending_moves_[loser_idx].start.x, pending_moves_[loser_idx].start.y);
    pending_moves_.erase(pending_moves_.begin() + static_cast<std::ptrdiff_t>(loser_idx));
}

void RealTimeArbiter::apply_yield(std::size_t yielder_idx, Position cell) {
    auto& yielder = pending_moves_[yielder_idx];
    auto path = get_path(yielder.start, yielder.dest);

    auto found = std::find(path.begin(), path.end(), cell);
    std::size_t idx = static_cast<std::size_t>(std::distance(path.begin(), found));

    if (idx <= 1) {
        pending_moves_.erase(pending_moves_.begin() + static_cast<std::ptrdiff_t>(yielder_idx));
    }
    else {
        Position stop_cell = path[idx - 1];
        yielder.dest = stop_cell;
        yielder.arrival_ms = yielder.scheduled_ms + get_distance(yielder.start, stop_cell) * move_ms_per_cell_;
    }
}

bool RealTimeArbiter::captures_king(const PendingMove& move) const {
    auto target = board_.get_at(move.dest.x, move.dest.y);
    return target.has_value() && target->type == PieceType::K && target->color != move.piece.color;
}

bool RealTimeArbiter::is_pawn_promotion(const PendingMove& move) const {
    if (move.piece.type != PieceType::P) return false;
    int last_row = (move.piece.color == Color::w) ? 0 : board_.get_height() - 1;
    return move.dest.y == last_row;
}