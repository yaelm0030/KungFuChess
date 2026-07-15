#pragma once

#include "GameSnapshot.h"
#include "ImageCache.h"
#include "Img.h"
#include "PieceAnimator.h"
#include "Position.h"

#include <optional>
#include <string>

class UIManager {
public:
    UIManager(ImageCache& images, std::string board_image_path);

    // dt_ms drives sprite-frame animation timing; pieces are drawn
    // interpolated between their origin and target cell per snapshot.progress.
    Img render(const GameSnapshot& snapshot, int dt_ms, std::optional<Position> selected_cell = std::nullopt);

private:
    ImageCache& images_;
    std::string board_image_path_;
    PieceAnimator animator_;
};
