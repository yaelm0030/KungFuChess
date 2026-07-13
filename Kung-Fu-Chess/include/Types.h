#pragma once

enum class Color {
    w,
    b
};

enum class PieceType {
    K, // King
    Q, // Queen
    R, // Rook
    B, // Bishop
    N, // Knight
    P  // Pawn
};

struct Cell {
    Color color;
    PieceType type;
    long long cooldown_end_ms = 0;

    bool is_on_cooldown(long long now_ms) const { return now_ms < cooldown_end_ms; }
};
