#pragma once

#include "GameSnapshot.h"
#include "GraphicsComponent.h"
#include "Types.h"

#include <string>
#include <unordered_map>

// Owns one GraphicsComponent per (piece type, color, state) combination and
// keeps it advancing in real time; every piece cycling through the same
// sprite sequence shares one clock, since which specific piece it is doesn't
// affect which frame is showing.
class PieceAnimator {
public:
    // Advances piece's current animation by dt_ms and returns its frame's
    // sprite path, loading frame paths and frames-per-second from disk the
    // first time a given type/color/state combination is seen.
    const std::string& frame_path(const PieceSnapshot& piece, int dt_ms);

private:
    GraphicsComponent& component_for(const PieceSnapshot& piece);

    std::unordered_map<int, GraphicsComponent> components_;
};
