#pragma once

#include <array>
#include <iostream>
#include <optional>

#include "GameEngine.h"
#include "Position.h"

class Controller {
public:
    explicit Controller(Board board, long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell);

    // nullopt acting_color uses a shared cursor (local play); a Color uses that color's own cursor.
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

    std::optional<Position>& cursor(std::optional<Color> acting_color);
    const std::optional<Position>& cursor(std::optional<Color> acting_color) const;

    // False only when the selected cell was stale; caller retries as a fresh click.
    bool handle_click_with_selection(Position cell, std::optional<Color> clicked_color, bool clicked_cell_is_selectable,
                                      std::optional<Position>& selection);
};
