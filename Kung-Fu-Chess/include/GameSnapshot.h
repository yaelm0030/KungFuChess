#pragma once

#include "Position.h"
#include "Types.h"

#include <optional>
#include <vector>

struct PixelPosition {
    int x;
    int y;
};

enum class PieceState {
    idle,
    move,
    jump,
    short_rest,
    long_rest
};

struct PieceSnapshot {
    PieceType type;
    Color color;
    PixelPosition pixels_location;        // origin cell while moving; current cell otherwise
    PixelPosition target_pixels_location; // destination cell while moving; equal to pixels_location otherwise
    double progress;                      // 0..1 fraction of the move elapsed; 1.0 when not moving
    PieceState state;
    double cooldown_progress = 0.0;       // 1.0 when cooldown just started, 0.0 when released
};

struct MoveRecord {
    PieceType type;
    Color color;
    Position source;
    Position destination;
};

struct GameSnapshot {
    int board_width;
    int board_height;
    std::vector<PieceSnapshot> pieces;
    bool is_game_over;
    int score_w = 0; // sum of captured black pieces' values
    int score_b = 0; // sum of captured white pieces' values
    std::vector<MoveRecord> move_history;
    std::optional<Position> selected; // always nullopt from GameEngine::snapshot(); Controller::snapshot() overlays it
};
