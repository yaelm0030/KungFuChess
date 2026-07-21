#pragma once

#include <array>
#include <iostream>
#include <optional>

#include "GameEngine.h"
#include "Position.h"

class Controller {
public:
    explicit Controller(Board board, long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell);

    // acting_color scopes the selection cursor: nullopt uses the shared unrestricted cursor
    // (local stdin/stdout play); a concrete Color uses that color's own cursor and may only
    // start a fresh selection or jump on a piece of that color.
    void click(int pixel_x, int pixel_y, std::optional<Color> acting_color = std::nullopt);
    void jump(int pixel_x, int pixel_y, std::optional<Color> acting_color = std::nullopt);
    void wait(int milliseconds);

    // Pieces mid-move still show at their origin.
    void print(std::ostream& out) const;

    bool has_selection(std::optional<Color> acting_color = std::nullopt) const { return cursor(acting_color).has_value(); }
    std::optional<Position> selected(std::optional<Color> acting_color = std::nullopt) const { return cursor(acting_color); }
    bool game_over() const { return engine_.game_over(); }
    GameSnapshot snapshot() const;
    const std::vector<MoveRecord>& move_history() const { return engine_.move_history(); }

private:
    GameEngine engine_;
    std::optional<Position> selected_;
    std::array<std::optional<Position>, 2> selected_by_color_;

    // Picks the selection cursor for acting_color: nullopt -> selected_, else selected_by_color_[color].
    std::optional<Position>& cursor(std::optional<Color> acting_color);
    const std::optional<Position>& cursor(std::optional<Color> acting_color) const;

    // False only when the selected cell was stale; caller retries as a fresh click.
    bool handle_click_with_selection(Position cell, std::optional<Color> clicked_color, bool clicked_cell_is_selectable,
                                      std::optional<Position>& selection);
};
