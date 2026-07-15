#include "PieceAnimator.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace {

const std::unordered_map<PieceType, char> kPieceTypeLetters{
    { PieceType::K, 'K' }, { PieceType::Q, 'Q' }, { PieceType::R, 'R' },
    { PieceType::B, 'B' }, { PieceType::N, 'N' }, { PieceType::P, 'P' },
};

const std::unordered_map<PieceState, std::string> kStateFolders{
    { PieceState::idle, "idle" },
    { PieceState::move, "move" },
    { PieceState::jump, "jump" },
    { PieceState::short_rest, "short_rest" },
    { PieceState::long_rest, "long_rest" },
};

char color_letter(Color color) {
    return color == Color::w ? 'W' : 'B';
}

std::string state_dir(PieceType type, Color color, PieceState state) {
    std::string piece_folder = std::string(1, kPieceTypeLetters.at(type)) + color_letter(color);
    return "assets/images/pieces/" + piece_folder + "/states/" + kStateFolders.at(state);
}

// Every "N.png" under dir/sprites, in order starting at 1, stopping at the
// first missing number.
std::vector<std::string> frame_paths_in(const std::string& dir) {
    std::vector<std::string> paths;
    for (int n = 1;; ++n) {
        std::string path = dir + "/sprites/" + std::to_string(n) + ".png";
        if (!std::filesystem::exists(path)) {
            break;
        }
        paths.push_back(path);
    }
    return paths;
}

// Pulls "frames_per_sec" out of dir/config.json. Not a general JSON parser -
// this is the only field this layer needs from that file.
int frames_per_sec_in(const std::string& dir) {
    std::ifstream config_file(dir + "/config.json");
    std::string text((std::istreambuf_iterator<char>(config_file)), std::istreambuf_iterator<char>());

    std::size_t key = text.find("\"frames_per_sec\"");
    if (key == std::string::npos) {
        throw std::runtime_error("missing frames_per_sec in " + dir + "/config.json");
    }
    std::size_t colon = text.find(':', key);
    return std::stoi(text.substr(colon + 1));
}

int state_key(PieceType type, Color color, PieceState state) {
    return (static_cast<int>(type) * 2 + static_cast<int>(color)) * 8 + static_cast<int>(state);
}

} // namespace

GraphicsComponent& PieceAnimator::component_for(const PieceSnapshot& piece) {
    int key = state_key(piece.type, piece.color, piece.state);
    auto it = components_.find(key);
    if (it != components_.end()) {
        return it->second;
    }

    std::string dir = state_dir(piece.type, piece.color, piece.state);
    int frame_duration_ms = 1000 / frames_per_sec_in(dir);
    return components_.emplace(key, GraphicsComponent(frame_paths_in(dir), frame_duration_ms)).first->second;
}

const std::string& PieceAnimator::frame_path(const PieceSnapshot& piece, int dt_ms) {
    GraphicsComponent& component = component_for(piece);
    component.update(dt_ms);
    return component.current_frame_path();
}
