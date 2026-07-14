#pragma once

#include "Types.h"

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
    PixelPosition pixels_location;
    PieceState state;
};

struct GameSnapshot {
    int board_width;
    int board_height;
    std::vector<PieceSnapshot> pieces;
    bool is_game_over;
};
