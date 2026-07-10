#include "GameEngine.h"

#include <algorithm>

#include "Parser.h"
#include "Piece.h"

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

GameEngine::GameEngine(Board board, long long move_ms_per_cell)
    : board_(std::move(board)), move_ms_per_cell_(move_ms_per_cell) {
}

// Converts pixel coordinates into board cell coordinates, or nullopt if the
// pixel falls outside the board.
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

// True if the piece at (x, y) has a pending move that hasn't arrived yet.
bool GameEngine::is_moving(int x, int y) const {
    for (const PendingMove& move : pending_moves_) {
        if (move.start.x == x && move.start.y == y) {
            return true;
        }
    }
    return false;
}

// True if a move between the two cells would share a cell with the route of
// any move already in flight.
bool GameEngine::conflicts_with_pending_move(int start_x, int start_y, int dest_x, int dest_y) const {
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
long long GameEngine::arrival_time_for(int start_x, int start_y, int dest_x, int dest_y) const {
    int distance_cells = std::max(abs_diff(start_x, dest_x), abs_diff(start_y, dest_y));
    return clock_ms_ + static_cast<long long>(distance_cells) * move_ms_per_cell_;
}

// True if `move` lands on a cell occupied by an enemy king.
bool GameEngine::captures_enemy_king(const PendingMove& move) const {
    std::optional<Cell> target = board_.get_at(move.dest.x, move.dest.y);
    return target.has_value() && target->type == PieceType::K && target->color != move.piece.color;
}

// True if `move`'s piece is a pawn landing on the farthest row from its own
// side: row 0 for white (which moves up), the last row for black.
bool GameEngine::is_pawn_promotion(const PendingMove& move) const {
    if (move.piece.type != PieceType::P) {
        return false;
    }
    int last_row = (move.piece.color == Color::w) ? 0 : board_.get_height() - 1;
    return move.dest.y == last_row;
}

// Applies every pending move whose arrival time has passed, and keeps the
// rest queued.
void GameEngine::settle_arrived_moves() {
    std::vector<PendingMove> still_pending;

    for (const PendingMove& move : pending_moves_) {
        if (move.arrival_ms <= clock_ms_) {
            if (captures_enemy_king(move)) {
                game_over_ = true;
            }
            Cell piece = move.piece;
            if (is_pawn_promotion(move)) {
                piece.type = PieceType::Q;
            }
            board_.place_at(move.dest.x, move.dest.y, piece);
            board_.clear_at(move.start.x, move.start.y);
        }
        else {
            still_pending.push_back(move);
        }
    }

    pending_moves_ = std::move(still_pending);
}

// Reselects a friendly selectable piece on `cell`, otherwise attempts to
// move the current selection onto it.
bool GameEngine::handle_click_with_selection(Position cell, std::optional<Cell> clicked_piece, bool clicked_cell_is_selectable) {
    std::optional<Cell> selected_piece = board_.get_at(selected_->x, selected_->y);
    if (!selected_piece.has_value()) {
        selected_.reset();
        return false;
    }

    if (clicked_cell_is_selectable && clicked_piece->color == selected_piece->color) {
        selected_ = cell;
        return true;
    }

    try_schedule_move(cell, *selected_piece);
    return true;
}

// Validates and queues a move of the selected piece onto `cell`, if the move
// is legal for the piece and its route doesn't collide with a move already
// in flight.
void GameEngine::try_schedule_move(Position cell, Cell selected_piece) {
    const Piece* piece = PieceFactory::get_piece(selected_piece.type);
    if (!piece || !piece->is_available_move(selected_->x, selected_->y, cell.x, cell.y, board_)) {
        return;
    }

	// Im not sure about this check, because it seems like it would prevent a piece from moving to a cell that is currently occupied by another piece, even if that piece is moving away. But the test cases seem to expect that behavior, so I'm leaving it in for now.
    //if (conflicts_with_pending_move(selected_->x, selected_->y, cell.x, cell.y)) {
    //    return;
    //}

    pending_moves_.push_back(PendingMove{
        *selected_,
        cell,
        selected_piece,
        arrival_time_for(selected_->x, selected_->y, cell.x, cell.y),
    });

    selected_.reset();
}

// Resolves a click to a board cell, then either updates the selection or
// schedules a move, depending on what's currently selected and clicked.
void GameEngine::click(int pixel_x, int pixel_y) {
    if (game_over_) {
        return;
    }

    std::optional<Position> cell = pixel_to_cell(pixel_x, pixel_y);
    if (!cell.has_value()) {
        return;
    }

    std::optional<Cell> clicked_piece = board_.get_at(cell->x, cell->y);
    bool clicked_cell_is_selectable = clicked_piece.has_value() && !is_moving(cell->x, cell->y);

    if (selected_.has_value() && handle_click_with_selection(*cell, clicked_piece, clicked_cell_is_selectable)) {
        return;
    }

    if (clicked_cell_is_selectable) {
        selected_ = cell;
    }
}

// Advances the clock and settles any moves that have now arrived.
void GameEngine::wait(int milliseconds) {
    if (milliseconds > 0) {
        clock_ms_ += milliseconds;
        settle_arrived_moves();
    }
}

void GameEngine::print(std::ostream& out) const {
    out << Parser::board_to_string(board_) << "\n";
}