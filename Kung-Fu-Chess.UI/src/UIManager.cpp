#include "UIManager.h"

#include "BoardMapper.h"
#include "Constants.h"
#include "Parser.h"

namespace {

int lerp(int from, int to, double t) {
    return static_cast<int>(from + (to - from) * t);
}

constexpr int kPanelColumnX[] = { 10, 80, 130, 190 };
constexpr int kPanelHeaderY = 20;
constexpr int kPanelRowHeightPx = 22;
const cv::Scalar kPanelTextColor(0, 0, 0, 255);

} // namespace

UIManager::UIManager(ImageCache& images, std::string board_image_path)
    : images_(images), board_image_path_(std::move(board_image_path)) {
}

Img UIManager::render(const GameSnapshot& snapshot, const std::vector<MoveRecord>& move_history, int dt_ms,
                       std::optional<Position> selected_cell) {
    const int board_px_w = snapshot.board_width * constants::kCellSizePx;
    const int board_px_h = snapshot.board_height * constants::kCellSizePx;
    const int board_offset_x = kHistoryPanelWidthPx;

    if (!resized_board_.has_value()) {
        Img board_only = images_.get(board_image_path_).clone();
        board_only.resize(board_px_w, board_px_h);

        Img canvas = Img::blank(kHistoryPanelWidthPx + board_px_w, board_px_h);
        board_only.draw_on(canvas, board_offset_x, 0);
        resized_board_ = canvas;
    }
    Img frame = resized_board_->clone();

    for (const auto& piece : snapshot.pieces) {
        int cell_x = board_offset_x + lerp(piece.pixels_location.x, piece.target_pixels_location.x, piece.progress);
        int cell_y = lerp(piece.pixels_location.y, piece.target_pixels_location.y, piece.progress);

        if (piece.state == PieceState::short_rest) {
            // Red fill drains from the top down as cooldown_progress falls, so the
            // remaining red always touches the bottom of the cell.
            int red_height = static_cast<int>(constants::kCellSizePx * piece.cooldown_progress);
            frame.draw_rectangle(cell_x, cell_y + constants::kCellSizePx - red_height, constants::kCellSizePx,
                                  red_height, cv::Scalar(0, 0, 255, 255), cv::FILLED);
        }

        Img sprite = images_.get(animator_.frame_path(piece, dt_ms)).clone();
        sprite.resize(constants::kCellSizePx, constants::kCellSizePx, /*keep_aspect=*/true);

        // Center the (possibly non-square) sprite within its cell.
        int x = cell_x + (constants::kCellSizePx - sprite.get_mat().cols) / 2;
        int y = cell_y + (constants::kCellSizePx - sprite.get_mat().rows) / 2;
        sprite.draw_on(frame, x, y);
    }

    if (selected_cell.has_value()) {
        frame.draw_rectangle(board_offset_x + selected_cell->x * constants::kCellSizePx,
                              selected_cell->y * constants::kCellSizePx, constants::kCellSizePx,
                              constants::kCellSizePx, cv::Scalar(0, 255, 255, 255), 3);
    }

    if (snapshot.is_game_over) {
        frame.put_text("Game Over", board_offset_x + 40, frame.get_mat().rows / 2, 1.5, cv::Scalar(0, 0, 255, 255), 3);
    }

    draw_move_history(frame, move_history, snapshot.board_height);

    return frame;
}

void UIManager::draw_move_history(Img& frame, const std::vector<MoveRecord>& move_history, int board_height) const {
    frame.put_text("Type", kPanelColumnX[0], kPanelHeaderY, 0.5, kPanelTextColor);
    frame.put_text("Color", kPanelColumnX[1], kPanelHeaderY, 0.5, kPanelTextColor);
    frame.put_text("From", kPanelColumnX[2], kPanelHeaderY, 0.5, kPanelTextColor);
    frame.put_text("To", kPanelColumnX[3], kPanelHeaderY, 0.5, kPanelTextColor);

    int y = kPanelHeaderY + kPanelRowHeightPx;
    for (const MoveRecord& record : move_history) {
        std::string token = Parser::token_from_cell(Cell{ record.color, record.type });
        std::string color_letter(1, token[0]);
        std::string type_letter(1, token[1]);
        std::string from = BoardMapper::cell_to_algebraic(record.source, board_height);
        std::string to = BoardMapper::cell_to_algebraic(record.destination, board_height);

        frame.put_text(type_letter, kPanelColumnX[0], y, 0.5, kPanelTextColor);
        frame.put_text(color_letter, kPanelColumnX[1], y, 0.5, kPanelTextColor);
        frame.put_text(from, kPanelColumnX[2], y, 0.5, kPanelTextColor);
        frame.put_text(to, kPanelColumnX[3], y, 0.5, kPanelTextColor);
        y += kPanelRowHeightPx;
    }
}
