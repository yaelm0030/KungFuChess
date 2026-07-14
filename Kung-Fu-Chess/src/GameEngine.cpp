#include "GameEngine.h"

#include "Parser.h"
#include "Piece.h"

GameEngine::GameEngine(Board board, long long move_ms_per_cell)
    : board_(std::move(board)), arbiter_(board_, move_ms_per_cell) {
}

bool GameEngine::is_selectable(Position cell) const {
    if (game_over_) {
        return false;
    }
    std::optional<Cell> piece = board_.get_at(cell.x, cell.y);
    return piece.has_value() && !arbiter_.is_moving(cell.x, cell.y) && !arbiter_.is_airborne(cell.x, cell.y) &&
           !piece->is_on_cooldown(arbiter_.clock_ms());
}

std::optional<Color> GameEngine::color_at(Position cell) const {
    std::optional<Cell> piece = board_.get_at(cell.x, cell.y);
    if (!piece.has_value()) {
        return std::nullopt;
    }
    return piece->color;
}

bool GameEngine::request_move(Position start, Position dest) {
    if (!is_selectable(start)) return false;

    std::optional<Cell> piece_at_start = board_.get_at(start.x, start.y);
    const Piece& piece = PieceFactory::get_piece(piece_at_start->type);
    if (!piece.is_available_move(start.x, start.y, dest.x, dest.y, board_)) return false;

    arbiter_.schedule_move(start, dest, *piece_at_start);
    return true;
}

bool GameEngine::request_jump(Position cell) {
    if (!is_selectable(cell)) return false;

    std::optional<Cell> piece = board_.get_at(cell.x, cell.y);
    arbiter_.start_jump(cell, *piece, kJumpDurationMs);
    return true;
}

// Ends the game if an enemy king was captured while settling.
void GameEngine::wait(int milliseconds) {
    if (milliseconds > 0 && arbiter_.advance(milliseconds)) {
        game_over_ = true;
    }
}

void GameEngine::print(std::ostream& out) const {
    out << Parser::board_to_string(board_) << "\n";
}
