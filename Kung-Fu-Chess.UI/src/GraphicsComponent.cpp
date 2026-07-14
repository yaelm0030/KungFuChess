#include "GraphicsComponent.h"

#include <stdexcept>

GraphicsComponent::GraphicsComponent(std::vector<std::string> frame_paths, int frame_duration_ms) {
    set_frames(std::move(frame_paths), frame_duration_ms);
}

void GraphicsComponent::update(int dt_ms) {
    elapsed_ms_ += dt_ms;
}

void GraphicsComponent::set_frames(std::vector<std::string> frame_paths, int frame_duration_ms) {
    if (frame_paths.empty() || frame_duration_ms <= 0) {
        throw std::invalid_argument("GraphicsComponent needs at least one frame and a positive frame duration.");
    }

    frame_paths_ = std::move(frame_paths);
    frame_duration_ms_ = frame_duration_ms;
    elapsed_ms_ = 0;
}

const std::string& GraphicsComponent::current_frame_path() const {
    int frame_index = (elapsed_ms_ / frame_duration_ms_) % static_cast<int>(frame_paths_.size());
    return frame_paths_[frame_index];
}
