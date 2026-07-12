#pragma once

#include "Board.h"

// SRP: centralizes move-validation logic (self-capture, path-blocking) that
// would otherwise be duplicated across every Piece subclass.
class RuleEngine {
public:
    static bool captures_own_color(int start_x, int start_y, int dest_x, int dest_y, const Board& board);

    // Only meaningful when start-dest is itself a straight or diagonal
    // line; callers must check that first.
    static bool is_path_clear(int start_x, int start_y, int dest_x, int dest_y, const Board& board);
};
