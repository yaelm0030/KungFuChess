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
    // Width, in pixels, reserved on the left for the move-history table;
    // InputHandler must subtract this from raw click coordinates before
    // mapping them onto the board.
    static constexpr int kHistoryPanelWidthPx = 260;

    UIManager(ImageCache& images, std::string board_image_path);

    // dt_ms drives sprite-frame animation timing; pieces are drawn
    // interpolated between their origin and target cell per snapshot.progress.
    Img render(const GameSnapshot& snapshot, const std::vector<MoveRecord>& move_history, int dt_ms,
               std::optional<Position> selected_cell = std::nullopt);

private:
    void draw_move_history(Img& frame, const std::vector<MoveRecord>& move_history, int board_height) const;

    ImageCache& images_;
    std::string board_image_path_;
    PieceAnimator animator_;
    std::optional<Img> resized_board_; // built once, at the first render()'s board size
};
