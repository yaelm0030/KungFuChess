#pragma once

#include "GameSnapshot.h"
#include "GraphicsComponent.h"
#include "Types.h"

#include <string>
#include <unordered_map>

// One GraphicsComponent per (type, color, state); all pieces in that state share one clock.
class PieceAnimator {
public:
    // Advances the animation by dt_ms; loads frames/fps from disk on first use.
    const std::string& frame_path(const PieceSnapshot& piece, int dt_ms);

private:
    GraphicsComponent& component_for(const PieceSnapshot& piece);

    std::unordered_map<int, GraphicsComponent> components_;
};
