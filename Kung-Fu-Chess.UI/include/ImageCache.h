#pragma once

#include "Img.h"

#include <string>
#include <unordered_map>

class ImageCache {
public:
    Img& get(const std::string& path);

private:
    std::unordered_map<std::string, Img> images_;
};
