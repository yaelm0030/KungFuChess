#include "Controller.h"

#include "BoardMapper.h"

Controller::Controller(Board board, long long move_ms_per_cell) : engine_(std::move(board), move_ms_per_cell) {
}

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

void Controller::click(int pixel_x, int pixel_y) {
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

void Controller::jump(int pixel_x, int pixel_y) {
    std::optional<Position> cell = BoardMapper::pixel_to_cell(pixel_x, pixel_y, engine_.width(), engine_.height());
    if (!cell.has_value()) {
        return;
    }

    if (engine_.request_jump(*cell)) {
        if (selected_.has_value() && *selected_ == *cell) {
            selected_.reset();
        }
    }
}

void Controller::wait(int milliseconds) {
    engine_.wait(milliseconds);
}

void Controller::print(std::ostream& out) const {
    engine_.print(out);
}
