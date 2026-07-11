#include "Controller.h"

#include "BoardMapper.h"

Controller::Controller(GameEngine& engine) : engine_(engine) {
}

// Reselects a friendly selectable piece on `cell`, otherwise attempts to
// move the current selection onto it.
bool Controller::handle_click_with_selection(Position cell, std::optional<Color> clicked_color, bool clicked_cell_is_selectable) {
    std::optional<Color> selected_color = engine_.color_at(*selected_);
    if (!selected_color.has_value()) {
        selected_.reset();
        return false;
    }

    if (clicked_cell_is_selectable && clicked_color == selected_color) {
        selected_ = cell;
        return true;
    }

    if (engine_.request_move(*selected_, cell)) {
        selected_.reset();
    }
    return true;
}

// Resolves a click to a board cell, then either updates the selection or
// requests a move, depending on what's currently selected and clicked.
void Controller::click(int pixel_x, int pixel_y) {
    if (engine_.game_over()) {
        return;
    }

    std::optional<Position> cell = BoardMapper::pixel_to_cell(pixel_x, pixel_y, engine_.width(), engine_.height());
    if (!cell.has_value()) {
        selected_.reset(); // clicking outside the board cancels the selection instantly
        return;
    }

    std::optional<Color> clicked_color = engine_.color_at(*cell);
    bool clicked_cell_is_selectable = engine_.is_selectable(*cell);

    if (selected_.has_value() && handle_click_with_selection(*cell, clicked_color, clicked_cell_is_selectable)) {
        return;
    }

    if (clicked_cell_is_selectable) {
        selected_ = cell;
    }
}

// Starts a jump for the piece at the given pixel, if one is eligible; drops
// the selection if the jumped piece was the current selection.
void Controller::jump(int pixel_x, int pixel_y) {
    if (engine_.game_over()) {
        return;
    }

    std::optional<Position> cell = BoardMapper::pixel_to_cell(pixel_x, pixel_y, engine_.width(), engine_.height());
    if (!cell.has_value()) {
        return;
    }

    if (engine_.request_jump(*cell)) {
        if (selected_.has_value() && selected_->x == cell->x && selected_->y == cell->y) {
            selected_.reset();
        }
    }
}
