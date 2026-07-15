#include "ImageCache.h"

Img& ImageCache::get(const std::string& path) {
    auto it = images_.find(path);
    if (it != images_.end()) {
        return it->second;
    }

    Img loaded;
    loaded.read(path);
    return images_.emplace(path, std::move(loaded)).first->second;
}
