#include "Piece.h"

#include <cstdlib>

namespace {

int abs_diff(int a, int b) {
    return std::abs(a - b);
}

// Walks the cells strictly between (start_x, start_y) and (dest_x, dest_y)
// along a straight or diagonal line, returning false if any is occupied.
// Only meaningful when (start_x, start_y)-(dest_x, dest_y) is itself a
// straight or diagonal line; callers must check that first.
bool is_path_clear(int start_x, int start_y, int dest_x, int dest_y, const Board& board) {
    int step_x = (dest_x > start_x) - (dest_x < start_x);
    int step_y = (dest_y > start_y) - (dest_y < start_y);

    int x = start_x + step_x;
    int y = start_y + step_y;

    while (x != dest_x || y != dest_y) {
        if (board.get_at(x, y).has_value()) {
            return false;
        }
        x += step_x;
        y += step_y;
    }
    return true;
}

// A move that lands on a piece of the same color as the mover is an illegal
// self-capture. A move to an empty cell, or onto an enemy piece, is fine.
bool captures_own_color(int start_x, int start_y, int dest_x, int dest_y, const Board& board) {
    std::optional<Cell> dest_cell = board.get_at(dest_x, dest_y);
    if (!dest_cell.has_value()) {
        return false;
    }
    std::optional<Cell> start_cell = board.get_at(start_x, start_y);
    return start_cell.has_value() && start_cell->color == dest_cell->color;
}

} // namespace

bool King::has_blockers(int, int, int, int, const Board&) const {
    return false; // a king only ever moves one cell; nothing can be "between"
}

bool King::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    if (!((dx != 0 || dy != 0) && dx <= 1 && dy <= 1)) {
        return false;
    }
    if (has_blockers(start_x, start_y, dest_x, dest_y, board)) {
        return false;
    }
    return !captures_own_color(start_x, start_y, dest_x, dest_y, board);
}

bool Rook::has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    if ((dx == 0) == (dy == 0)) {
        return false; // not a straight line; blocking is undefined
    }
    return !is_path_clear(start_x, start_y, dest_x, dest_y, board);
}

bool Rook::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    if ((dx == 0) == (dy == 0)) {
        return false; // no movement, or not aligned on a single axis
    }
    if (has_blockers(start_x, start_y, dest_x, dest_y, board)) {
        return false;
    }
    return !captures_own_color(start_x, start_y, dest_x, dest_y, board);
}

bool Bishop::has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    if (dx == 0 || dx != dy) {
        return false; // not a diagonal; blocking is undefined
    }
    return !is_path_clear(start_x, start_y, dest_x, dest_y, board);
}

bool Bishop::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    if (dx == 0 || dx != dy) {
        return false;
    }
    if (has_blockers(start_x, start_y, dest_x, dest_y, board)) {
        return false;
    }
    return !captures_own_color(start_x, start_y, dest_x, dest_y, board);
}

bool Queen::has_blockers(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    bool straight = (dx == 0) != (dy == 0);
    bool diagonal = (dx == dy) && dx != 0;
    if (!straight && !diagonal) {
        return false; // not a straight or diagonal line; blocking is undefined
    }
    return !is_path_clear(start_x, start_y, dest_x, dest_y, board);
}

bool Queen::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    bool straight = (dx == 0) != (dy == 0);
    bool diagonal = (dx == dy) && dx != 0;
    if (!straight && !diagonal) {
        return false;
    }
    if (has_blockers(start_x, start_y, dest_x, dest_y, board)) {
        return false;
    }
    return !captures_own_color(start_x, start_y, dest_x, dest_y, board);
}

bool Knight::has_blockers(int, int, int, int, const Board&) const {
    return false; // knights jump over any piece in between
}

bool Knight::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    if (!((dx == 1 && dy == 2) || (dx == 2 && dy == 1))) {
        return false;
    }
    if (has_blockers(start_x, start_y, dest_x, dest_y, board)) {
        return false;
    }
    return !captures_own_color(start_x, start_y, dest_x, dest_y, board);
}

bool Pawn::has_blockers(int, int, int, int, const Board&) const {
    return false; // a pawn only ever moves one cell; nothing can be "between"
}

bool Pawn::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    std::optional<Cell> start_cell = board.get_at(start_x, start_y);
    if (!start_cell.has_value()) {
        return false;
    }

    int dx = dest_x - start_x;
    int dy = dest_y - start_y;
    int forward = (start_cell->color == Color::w) ? -1 : 1; // white moves up (-y), black moves down (+y)

    std::optional<Cell> dest_cell = board.get_at(dest_x, dest_y);

    if (dx == 0 && dy == forward) {
        return !dest_cell.has_value(); // one cell forward, only onto an empty cell
    }

    if (std::abs(dx) == 1 && dy == forward) {
        return dest_cell.has_value() && dest_cell->color != start_cell->color; // diagonal capture only
    }

    return false; // two-cell moves and forward captures are not allowed
}

const Piece* PieceFactory::get_piece(PieceType type) {
    static King king;
    static Queen queen;
    static Rook rook;
    static Bishop bishop;
    static Knight knight;
    static Pawn pawn;

    switch (type) {
        case PieceType::K: return &king;
        case PieceType::Q: return &queen;
        case PieceType::R: return &rook;
        case PieceType::B: return &bishop;
        case PieceType::N: return &knight;
        case PieceType::P: return &pawn;
        default: return nullptr;
    }
}
