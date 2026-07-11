#pragma once

#include <optional>

#include "GameEngine.h"
#include "Position.h"

// Translates pixel-space clicks into logical move/jump requests on a
// GameEngine, and owns the UI-facing selection state. Knows nothing about
// chess rules: it only decides whether a click selects a piece, replaces
// the selection, or asks the engine to move the current selection.
class Controller {
public:
    explicit Controller(GameEngine& engine);

    // Converts the pixel to a board cell and applies the click rules:
    // selects a piece, replaces the selection with another friendly piece,
    // or asks the engine to move the selection there. A click outside the
    // board cancels any active selection instantly. A move request the
    // engine rejects (illegal shape, blocked path, or colliding with a move
    // already in flight) leaves the current selection in place, so a
    // different destination can be tried without reselecting.
    void click(int pixel_x, int pixel_y);

    // Converts the pixel to a board cell and asks the engine to start a
    // jump there. If the jumped piece was the current selection, the
    // selection is dropped, since an airborne piece cannot be moved.
    void jump(int pixel_x, int pixel_y);

    bool has_selection() const { return selected_.has_value(); }
    std::optional<Position> selected() const { return selected_; }

private:
    GameEngine& engine_;
    std::optional<Position> selected_;

    // Handles a click while a piece is already selected: reselects on a
    // click on another selectable friendly piece, otherwise attempts to
    // move the selection to `cell`. Returns false only when the selected
    // cell turned out to be stale (its piece is gone), in which case the
    // selection is cleared and the click should be treated as a fresh one.
    bool handle_click_with_selection(Position cell, std::optional<Color> clicked_color, bool clicked_cell_is_selectable);
};
