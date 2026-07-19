#include "UIManager.h"

#include "BoardMapper.h"
#include "Constants.h"
#include "Parser.h"
#include "UIConstants.h"

namespace {

int lerp(int from, int to, double t) {
    return static_cast<int>(from + (to - from) * t);
}

} // namespace

UIManager::UIManager(ImageCache& images, std::string board_image_path)
    : images_(images), board_image_path_(std::move(board_image_path)) {
}

Img UIManager::render(const GameSnapshot& snapshot, const std::vector<MoveRecord>& move_history, int dt_ms,
                       std::optional<Position> selected_cell) {
    const int board_px_w = snapshot.board_width * constants::kCellSizePx;
    const int board_px_h = snapshot.board_height * constants::kCellSizePx;

    if (!resized_board_.has_value()) {
        Img board_only = images_.get(board_image_path_).clone();
        board_only.resize(board_px_w, board_px_h);

        Img canvas = Img::blank(ui_constants::kBoardOffsetX + board_px_w, board_px_h + ui_constants::kFileLabelMarginPx,
                                 ui_constants::kPanelBackgroundColor);
        board_only.draw_on(canvas, ui_constants::kBoardOffsetX, 0);
        resized_board_ = canvas;
    }
    Img frame = resized_board_->clone();

    draw_pieces(frame, snapshot.pieces, dt_ms);

    if (selected_cell.has_value()) {
        draw_selection_highlight(frame, *selected_cell);
    }

    if (snapshot.is_game_over) {
        draw_game_over_message(frame, board_px_h);
    }

    draw_axis_labels(frame, snapshot.board_width, snapshot.board_height);
    draw_score(frame, snapshot.score_w, snapshot.score_b);
    draw_move_history(frame, move_history, snapshot.board_height);

    return frame;
}

void UIManager::draw_pieces(Img& frame, const std::vector<PieceSnapshot>& pieces, int dt_ms) {
    for (const auto& piece : pieces) {
        int cell_x =
            ui_constants::kBoardOffsetX + lerp(piece.pixels_location.x, piece.target_pixels_location.x, piece.progress);
        int cell_y = lerp(piece.pixels_location.y, piece.target_pixels_location.y, piece.progress);

        if (piece.state == PieceState::short_rest) {
            draw_cooldown_overlay(frame, cell_x, cell_y, piece.cooldown_progress);
        }

        Img sprite = images_.get(animator_.frame_path(piece, dt_ms)).clone();
        sprite.resize(constants::kCellSizePx, constants::kCellSizePx, /*keep_aspect=*/true);

        // Center the (possibly non-square) sprite within its cell.
        int x = cell_x + (constants::kCellSizePx - sprite.get_mat().cols) / 2;
        int y = cell_y + (constants::kCellSizePx - sprite.get_mat().rows) / 2;
        sprite.draw_on(frame, x, y);
    }
}

void UIManager::draw_game_over_message(Img& frame, int board_px_h) const {
    frame.put_text("Game Over", ui_constants::kBoardOffsetX + 40, board_px_h / 2, ui_constants::kGameOverFontSize,
                    ui_constants::kGameOverColor, ui_constants::kGameOverThickness);
}

void UIManager::draw_cooldown_overlay(Img& frame, int cell_x, int cell_y, double cooldown_progress) const {
    // Red fill drains from the top down as cooldown_progress falls, so the
    // remaining red always touches the bottom of the cell.
    int red_height = static_cast<int>(constants::kCellSizePx * cooldown_progress);
    frame.draw_rectangle(cell_x, cell_y + constants::kCellSizePx - red_height, constants::kCellSizePx, red_height,
                          ui_constants::kCooldownFillColor, cv::FILLED);
}

void UIManager::draw_selection_highlight(Img& frame, Position selected_cell) const {
    frame.draw_rectangle(ui_constants::kBoardOffsetX + selected_cell.x * constants::kCellSizePx,
                          selected_cell.y * constants::kCellSizePx, constants::kCellSizePx, constants::kCellSizePx,
                          ui_constants::kSelectionColor, ui_constants::kSelectionThickness);
}

void UIManager::draw_score(Img& frame, int score_w, int score_b) const {
    frame.put_text("White: " + std::to_string(score_w), ui_constants::kHistoryColumnX[0], ui_constants::kScoreY,
                    ui_constants::kScoreFontSize, ui_constants::kTextColor);
    frame.put_text("Black: " + std::to_string(score_b), ui_constants::kHistoryColumnX[0],
                    ui_constants::kScoreY + ui_constants::kScoreRowHeightPx, ui_constants::kScoreFontSize,
                    ui_constants::kTextColor);
}

void UIManager::draw_axis_labels(Img& frame, int board_width, int board_height) const {
    for (int x = 0; x < board_width; ++x) {
        std::string file(1, static_cast<char>('a' + x));
        int center_x = ui_constants::kBoardOffsetX + x * constants::kCellSizePx + constants::kCellSizePx / 2;
        frame.put_text(file, center_x - 5, board_height * constants::kCellSizePx + ui_constants::kFileLabelMarginPx - 8,
                        ui_constants::kAxisLabelFontSize, ui_constants::kTextColor);
    }

    for (int y = 0; y < board_height; ++y) {
        std::string rank = std::to_string(board_height - y);
        int center_y = y * constants::kCellSizePx + constants::kCellSizePx / 2;
        frame.put_text(rank, ui_constants::kBoardOffsetX - ui_constants::kRankLabelMarginPx + 8, center_y + 5,
                        ui_constants::kAxisLabelFontSize, ui_constants::kTextColor);
    }
}

void UIManager::draw_move_history(Img& frame, const std::vector<MoveRecord>& move_history, int board_height) const {
    frame.put_text("Type", ui_constants::kHistoryColumnX[0], ui_constants::kHistoryHeaderY, ui_constants::kHistoryFontSize,
                    ui_constants::kTextColor);
    frame.put_text("Color", ui_constants::kHistoryColumnX[1], ui_constants::kHistoryHeaderY,
                    ui_constants::kHistoryFontSize, ui_constants::kTextColor);
    frame.put_text("From", ui_constants::kHistoryColumnX[2], ui_constants::kHistoryHeaderY,
                    ui_constants::kHistoryFontSize, ui_constants::kTextColor);
    frame.put_text("To", ui_constants::kHistoryColumnX[3], ui_constants::kHistoryHeaderY, ui_constants::kHistoryFontSize,
                    ui_constants::kTextColor);

    int y = ui_constants::kHistoryHeaderY + ui_constants::kHistoryRowHeightPx;
    for (const MoveRecord& record : move_history) {
        std::string token = Parser::token_from_cell(Cell{ record.color, record.type });
        std::string color_letter(1, token[0]);
        std::string type_letter(1, token[1]);
        std::string from = BoardMapper::cell_to_algebraic(record.source, board_height);
        std::string to = BoardMapper::cell_to_algebraic(record.destination, board_height);

        frame.put_text(type_letter, ui_constants::kHistoryColumnX[0], y, ui_constants::kHistoryFontSize,
                        ui_constants::kTextColor);
        frame.put_text(color_letter, ui_constants::kHistoryColumnX[1], y, ui_constants::kHistoryFontSize,
                        ui_constants::kTextColor);
        frame.put_text(from, ui_constants::kHistoryColumnX[2], y, ui_constants::kHistoryFontSize,
                        ui_constants::kTextColor);
        frame.put_text(to, ui_constants::kHistoryColumnX[3], y, ui_constants::kHistoryFontSize,
                        ui_constants::kTextColor);
        y += ui_constants::kHistoryRowHeightPx;
    }
}
