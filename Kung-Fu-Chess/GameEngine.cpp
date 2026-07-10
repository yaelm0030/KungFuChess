#include "GameEngine.h"

#include <algorithm>

#include "Parser.h"
#include "Piece.h"

namespace {

int abs_diff(int a, int b) {
    return a > b ? a - b : b - a;
}

} // namespace

GameEngine::GameEngine(Board board, long long move_ms_per_cell)
    : board_(std::move(board)), move_ms_per_cell_(move_ms_per_cell) {}

std::optional<Position> GameEngine::pixel_to_cell(int pixel_x, int pixel_y) const {
    if (pixel_x < 0 || pixel_y < 0) {
        return std::nullopt;
    }

    int cell_x = pixel_x / kCellSizePx;
    int cell_y = pixel_y / kCellSizePx;

    if (cell_x >= board_.get_width() || cell_y >= board_.get_height()) {
        return std::nullopt;
    }

    return Position{ cell_x, cell_y };
}

bool GameEngine::has_pending_move_from(int x, int y) const {
    for (const PendingMove& move : pending_moves_) {
        if (move.start.x == x && move.start.y == y) {
            return true;
        }
    }
    return false;
}

long long GameEngine::arrival_time_for(int start_x, int start_y, int dest_x, int dest_y) const {
    int distance_cells = std::max(abs_diff(start_x, dest_x), abs_diff(start_y, dest_y));
    return clock_ms_ + static_cast<long long>(distance_cells) * move_ms_per_cell_;
}

void GameEngine::settle_arrived_moves() {
    std::vector<PendingMove> still_pending;
    for (const PendingMove& move : pending_moves_) {
        if (move.arrival_ms <= clock_ms_) {
            board_.place_at(move.dest.x, move.dest.y, move.piece);
            board_.clear_at(move.start.x, move.start.y);
        } else {
            still_pending.push_back(move);
        }
    }
    pending_moves_ = std::move(still_pending);
}

void GameEngine::click(int pixel_x, int pixel_y) {
    std::optional<Position> cell = pixel_to_cell(pixel_x, pixel_y);
    if (!cell.has_value()) {
        return;
    }

    std::optional<Cell> clicked_piece = board_.get_at(cell->x, cell->y);
    bool clicked_cell_is_selectable = clicked_piece.has_value() && !has_pending_move_from(cell->x, cell->y);

    if (selected_.has_value()) {
        std::optional<Cell> selected_piece = board_.get_at(selected_->x, selected_->y);
        if (selected_piece.has_value()) {
            if (clicked_cell_is_selectable && clicked_piece->color == selected_piece->color) {
                selected_ = cell;
                return;
            }

            const Piece* piece = PieceFactory::get_piece(selected_piece->type);
            if (!piece || !piece->is_available_move(selected_->x, selected_->y, cell->x, cell->y, board_)) {
                // The move doesn't match the piece's shape (or the path is
                // blocked); ignore the click and keep the current selection.
                return;
            }

            pending_moves_.push_back(PendingMove{
                *selected_,
                *cell,
                *selected_piece,
                arrival_time_for(selected_->x, selected_->y, cell->x, cell->y),
            });
            selected_.reset();
            return;
        }
        // The selected cell no longer holds a piece; drop the stale selection
        // and treat this click as a fresh one below.
        selected_.reset();
    }

    if (clicked_cell_is_selectable) {
        selected_ = cell;
    }
}

void GameEngine::wait(int milliseconds) {
    if (milliseconds > 0) {
        clock_ms_ += milliseconds;
        settle_arrived_moves();
    }
}

void GameEngine::print(std::ostream& out) const {
    out << Parser::board_to_string(board_) << "\n";
}
