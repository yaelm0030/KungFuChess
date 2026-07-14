#pragma once

#include "GameSnapshot.h"
#include "ImageCache.h"
#include "Img.h"

#include <string>

class UIManager {
public:
    UIManager(ImageCache& images, std::string board_image_path);

    Img render(const GameSnapshot& snapshot);

private:
    ImageCache& images_;
    std::string board_image_path_;
};
