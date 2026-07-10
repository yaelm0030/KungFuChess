#include "Piece.h"

#include <cstdlib>

namespace {

int abs_diff(int a, int b) {
    return std::abs(a - b);
}

// Walks the cells strictly between (start_x, start_y) and (dest_x, dest_y)
// along a straight or diagonal line, returning false if any is occupied.
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

} // namespace

bool King::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board&) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    return (dx != 0 || dy != 0) && dx <= 1 && dy <= 1;
}

bool Rook::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    if ((dx == 0) == (dy == 0)) {
        return false; // no movement, or not aligned on a single axis
    }
    return is_path_clear(start_x, start_y, dest_x, dest_y, board);
}

bool Bishop::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    if (dx == 0 || dx != dy) {
        return false;
    }
    return is_path_clear(start_x, start_y, dest_x, dest_y, board);
}

bool Queen::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board& board) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    bool straight = (dx == 0) != (dy == 0);
    bool diagonal = (dx == dy) && dx != 0;
    if (!straight && !diagonal) {
        return false;
    }
    return is_path_clear(start_x, start_y, dest_x, dest_y, board);
}

bool Knight::is_available_move(int start_x, int start_y, int dest_x, int dest_y, const Board&) const {
    int dx = abs_diff(start_x, dest_x);
    int dy = abs_diff(start_y, dest_y);
    return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
}

const Piece* PieceFactory::get_piece(PieceType type) {
    static King king;
    static Queen queen;
    static Rook rook;
    static Bishop bishop;
    static Knight knight;

    switch (type) {
        case PieceType::K: return &king;
        case PieceType::Q: return &queen;
        case PieceType::R: return &rook;
        case PieceType::B: return &bishop;
        case PieceType::N: return &knight;
        default: return nullptr;
    }
}
