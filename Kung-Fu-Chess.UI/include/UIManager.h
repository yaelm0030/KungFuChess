#pragma once

#include "GameSnapshot.h"
#include "ImageCache.h"
#include "Img.h"
#include "Position.h"

#include <optional>
#include <string>

class UIManager {
public:
    UIManager(ImageCache& images, std::string board_image_path);

    Img render(const GameSnapshot& snapshot, std::optional<Position> selected_cell = std::nullopt);

private:
    ImageCache& images_;
    std::string board_image_path_;
};
