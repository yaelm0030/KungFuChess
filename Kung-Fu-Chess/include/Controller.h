#pragma once

#include <iostream>
#include <optional>

#include "GameEngine.h"
#include "Position.h"

class Controller {
public:
    explicit Controller(Board board, long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell);

    void click(int pixel_x, int pixel_y);
    void jump(int pixel_x, int pixel_y);
    void wait(int milliseconds);

    // Pieces mid-move still show at their origin.
    void print(std::ostream& out) const;

    bool has_selection() const { return selected_.has_value(); }
    std::optional<Position> selected() const { return selected_; }
    bool game_over() const { return engine_.game_over(); }

private:
    GameEngine engine_;
    std::optional<Position> selected_;

    // False only when the selected cell was stale; caller retries as a fresh click.
    bool handle_click_with_selection(Position cell, std::optional<Color> clicked_color, bool clicked_cell_is_selectable);
};
