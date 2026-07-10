#include "GameEngine.h"

#include "Parser.h"

GameEngine::GameEngine(Board board) : board_(std::move(board)) {}

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

void GameEngine::click(int pixel_x, int pixel_y) {
    std::optional<Position> cell = pixel_to_cell(pixel_x, pixel_y);
    if (!cell.has_value()) {
        return;
    }

    std::optional<Cell> clicked_piece = board_.get_at(cell->x, cell->y);

    if (selected_.has_value()) {
        std::optional<Cell> selected_piece = board_.get_at(selected_->x, selected_->y);
        if (selected_piece.has_value()) {
            if (clicked_piece.has_value() && clicked_piece->color == selected_piece->color) {
                selected_ = cell;
            } else {
                board_.place_at(cell->x, cell->y, *selected_piece);
                board_.clear_at(selected_->x, selected_->y);
                selected_.reset();
            }
            return;
        }
        // The selected cell no longer holds a piece; drop the stale selection
        // and treat this click as a fresh one below.
        selected_.reset();
    }

    if (clicked_piece.has_value()) {
        selected_ = cell;
    }
}

void GameEngine::wait(int milliseconds) {
    if (milliseconds > 0) {
        clock_ms_ += milliseconds;
    }
}

void GameEngine::print(std::ostream& out) const {
    out << Parser::board_to_string(board_) << "\n";
}
