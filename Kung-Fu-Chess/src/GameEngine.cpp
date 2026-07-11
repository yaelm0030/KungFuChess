#include "GameEngine.h"

#include "Parser.h"
#include "Piece.h"

GameEngine::GameEngine(Board board, long long move_ms_per_cell)
    : board_(std::move(board)), arbiter_(board_, move_ms_per_cell) {
}

bool GameEngine::is_selectable(Position cell) const {
    std::optional<Cell> piece = board_.get_at(cell.x, cell.y);
    return piece.has_value() && !arbiter_.is_moving(cell.x, cell.y) && !arbiter_.is_airborne(cell.x, cell.y);
}

std::optional<Color> GameEngine::color_at(Position cell) const {
    std::optional<Cell> piece = board_.get_at(cell.x, cell.y);
    if (!piece.has_value()) {
        return std::nullopt;
    }
    return piece->color;
}

// Validates and queues a move of the piece at `start` onto `dest`, if the
// move is legal for that piece and doesn't collide with a move already in
// flight.
bool GameEngine::request_move(Position start, Position dest) {
    if (game_over_) {
        return false;
    }

    std::optional<Cell> piece_at_start = board_.get_at(start.x, start.y);
    if (!piece_at_start.has_value()) {
        return false;
    }

    // An airborne piece is committed to its jump and stays on its cell; it
    // cannot be moved until it lands.
    if (arbiter_.is_airborne(start.x, start.y)) {
        return false;
    }

    const Piece* piece = PieceFactory::get_piece(piece_at_start->type);
    if (!piece || !piece->is_available_move(start.x, start.y, dest.x, dest.y, board_)) {
        return false;
    }

    // Reject a move whose route overlaps a move already in flight, to avoid
    // scheduling overlapping real-time commands on the same cells.
    if (arbiter_.conflicts_with_pending_move(start.x, start.y, dest.x, dest.y)) {
        return false;
    }

    arbiter_.schedule_move(start, dest, *piece_at_start);
    return true;
}

// Starts a jump for the piece at `cell`, if one is eligible: it must be
// present, not already moving, and not already airborne.
bool GameEngine::request_jump(Position cell) {
    if (game_over_) {
        return false;
    }

    std::optional<Cell> piece = board_.get_at(cell.x, cell.y);
    if (!piece.has_value() || arbiter_.is_moving(cell.x, cell.y) || arbiter_.is_airborne(cell.x, cell.y)) {
        return false;
    }

    arbiter_.start_jump(cell, *piece, kJumpDurationMs);
    return true;
}

// Advances the clock and settles any moves that have now arrived; ends the
// game if an enemy king was captured while settling.
void GameEngine::wait(int milliseconds) {
    if (milliseconds > 0 && arbiter_.advance(milliseconds)) {
        game_over_ = true;
    }
}

void GameEngine::print(std::ostream& out) const {
    out << Parser::board_to_string(board_) << "\n";
}
