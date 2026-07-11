#include "RuleEngine.h"

bool RuleEngine::captures_own_color(int start_x, int start_y, int dest_x, int dest_y, const Board& board) {
    std::optional<Cell> dest_cell = board.get_at(dest_x, dest_y);
    if (!dest_cell.has_value()) {
        return false;
    }
    std::optional<Cell> start_cell = board.get_at(start_x, start_y);
    return start_cell.has_value() && start_cell->color == dest_cell->color;
}

bool RuleEngine::is_path_clear(int start_x, int start_y, int dest_x, int dest_y, const Board& board) {
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
