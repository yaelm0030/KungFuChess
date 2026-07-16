#include "GameEngine.h"

#include <algorithm>

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

GameSnapshot GameEngine::snapshot() const {
    GameSnapshot snap{ board_.get_width(), board_.get_height(), {}, game_over_ };

    for (int y = 0; y < board_.get_height(); ++y) {
        for (int x = 0; x < board_.get_width(); ++x) {
            std::optional<Cell> cell = board_.get_at(x, y);
            if (!cell.has_value()) {
                continue;
            }

            PieceState state = PieceState::idle;
            PixelPosition origin{ x * constants::kCellSizePx, y * constants::kCellSizePx };
            PixelPosition target = origin;
            double progress = 1.0;
            double cooldown_progress = 0.0;

            if (arbiter_.is_airborne(x, y)) {
                state = PieceState::jump;
            } else if (std::optional<RealTimeArbiter::MoveProgress> move = arbiter_.move_progress_at(x, y)) {
                state = PieceState::move;
                target = PixelPosition{ move->dest.x * constants::kCellSizePx, move->dest.y * constants::kCellSizePx };
                long long total_ms = move->arrival_ms - move->scheduled_ms;
                progress = total_ms > 0 ? std::clamp(static_cast<double>(clock_ms() - move->scheduled_ms) /
                                                          static_cast<double>(total_ms),
                                                      0.0, 1.0)
                                        : 1.0;
            } else if (cell->is_on_cooldown(arbiter_.clock_ms())) {
                state = PieceState::short_rest;
                cooldown_progress = std::clamp(static_cast<double>(cell->cooldown_end_ms - arbiter_.clock_ms()) /
                                                    static_cast<double>(kCooldownMs),
                                                0.0, 1.0);
            }

            snap.pieces.push_back(PieceSnapshot{
                cell->type,
                cell->color,
                origin,
                target,
                progress,
                state,
                cooldown_progress,
            });
        }
    }

    return snap;
}
