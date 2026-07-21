#include "Controller.h"

#include "BoardMapper.h"

Controller::Controller(Board board, long long move_ms_per_cell) : engine_(std::move(board), move_ms_per_cell) {
}

std::optional<Position>& Controller::cursor(std::optional<Color> acting_color) {
    if (!acting_color.has_value()) {
        return selected_;
    }
    return selected_by_color_[static_cast<size_t>(*acting_color)];
}

const std::optional<Position>& Controller::cursor(std::optional<Color> acting_color) const {
    if (!acting_color.has_value()) {
        return selected_;
    }
    return selected_by_color_[static_cast<size_t>(*acting_color)];
}

bool Controller::handle_click_with_selection(Position cell, std::optional<Color> clicked_color, bool clicked_cell_is_selectable,
                                              std::optional<Position>& selection) {
    std::optional<Color> selected_color = engine_.color_at(*selection);
    if (!selected_color.has_value()) {
        selection.reset();
        return false;
    }

    if (clicked_cell_is_selectable && clicked_color == selected_color) {
        selection = cell;
        return true;
    }

    engine_.request_move(*selection, cell);
    selection.reset();
    return true;
}

void Controller::click(int pixel_x, int pixel_y, std::optional<Color> acting_color) {
    std::optional<Position> cell = BoardMapper::pixel_to_cell(pixel_x, pixel_y, engine_.width(), engine_.height());
    std::optional<Position>& selection = cursor(acting_color);
    if (!cell.has_value()) {
        selection.reset(); // clicking outside the board cancels the selection instantly
        return;
    }

    std::optional<Color> clicked_color = engine_.color_at(*cell);
    bool clicked_cell_is_selectable = engine_.is_selectable(*cell);

    if (selection.has_value() && handle_click_with_selection(*cell, clicked_color, clicked_cell_is_selectable, selection)) {
        return;
    }

    // A fresh selection under a concrete acting_color may only pick up that color's own piece.
    if (clicked_cell_is_selectable && (!acting_color.has_value() || clicked_color == acting_color)) {
        selection = cell;
    }
}

void Controller::jump(int pixel_x, int pixel_y, std::optional<Color> acting_color) {
    std::optional<Position> cell = BoardMapper::pixel_to_cell(pixel_x, pixel_y, engine_.width(), engine_.height());
    if (!cell.has_value()) {
        return;
    }

    if (acting_color.has_value() && engine_.color_at(*cell) != acting_color) {
        return;
    }

    if (engine_.request_jump(*cell)) {
        std::optional<Position>& selection = cursor(acting_color);
        if (selection.has_value() && *selection == *cell) {
            selection.reset();
        }
    }
}

void Controller::wait(int milliseconds) {
    engine_.wait(milliseconds);
}

void Controller::print(std::ostream& out) const {
    engine_.print(out);
}

// Overlays the controller-owned selection cursor onto the engine's snapshot; GameEngine has no concept of UI selection.
GameSnapshot Controller::snapshot() const {
    GameSnapshot snap = engine_.snapshot();
    snap.selected = selected_;
    return snap;
}
