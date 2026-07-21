#pragma once

#include "GameSnapshot.h"
#include "ImageCache.h"
#include "Img.h"
#include "PieceAnimator.h"
#include "Position.h"

#include <optional>
#include <string>
#include <vector>

class UIManager {
public:
    UIManager(ImageCache& images, std::string board_image_path);

    // dt_ms drives sprite-frame animation timing; pieces are drawn
    // interpolated between their origin and target cell per snapshot.progress.
    // move_history/selected come from the snapshot itself, not separate params,
    // since a networked GameSnapshot already carries both.
    Img render(const GameSnapshot& snapshot, int dt_ms);

private:
    void draw_pieces(Img& frame, const std::vector<PieceSnapshot>& pieces, int dt_ms);
    void draw_move_history(Img& frame, const std::vector<MoveRecord>& move_history, int board_height) const;
    void draw_axis_labels(Img& frame, int board_width, int board_height) const;
    void draw_score(Img& frame, int score_w, int score_b) const;
    void draw_game_over_message(Img& frame, int board_px_h) const;
    void draw_cooldown_overlay(Img& frame, int cell_x, int cell_y, double cooldown_progress) const;
    void draw_selection_highlight(Img& frame, Position selected_cell) const;

    ImageCache& images_;
    std::string board_image_path_;
    PieceAnimator animator_;
    std::optional<Img> resized_board_; // built once, at the first render()'s board size
};
