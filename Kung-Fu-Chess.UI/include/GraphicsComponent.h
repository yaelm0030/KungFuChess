#pragma once

#include <string>
#include <vector>

class GraphicsComponent {
public:
    // frame_paths non-empty, frame_duration_ms > 0, else throws std::invalid_argument.
    GraphicsComponent(std::vector<std::string> frame_paths, int frame_duration_ms);

    void update(int dt_ms);

    void set_frames(std::vector<std::string> frame_paths, int frame_duration_ms);

    const std::string& current_frame_path() const;

private:
    std::vector<std::string> frame_paths_;
    int frame_duration_ms_;
    int elapsed_ms_ = 0;
};
