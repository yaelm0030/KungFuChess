#pragma once

#include "Board.h"

class RuleEngine {
public:
    // A move that lands on a piece of the same color as the mover is an
    // illegal self-capture. A move to an empty cell, or onto an enemy
    // piece, is fine.
    static bool captures_own_color(int start_x, int start_y, int dest_x, int dest_y, const Board& board);

    // Walks the cells strictly between (start_x, start_y) and (dest_x, dest_y)
    // along a straight or diagonal line, returning false if any is occupied.
    // Only meaningful when (start_x, start_y)-(dest_x, dest_y) is itself a
    // straight or diagonal line; callers must check that first. Pieces only
    // decide *whether* a given move shape can be blocked (e.g. a rook only
    // cares about straight lines); the physical board-occupancy check for
    // that line is this validation service's responsibility.
    static bool is_path_clear(int start_x, int start_y, int dest_x, int dest_y, const Board& board);
};
