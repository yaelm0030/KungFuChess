#include "UIManager.h"

#include "Constants.h"

namespace {

int lerp(int from, int to, double t) {
    return static_cast<int>(from + (to - from) * t);
}

} // namespace

UIManager::UIManager(ImageCache& images, std::string board_image_path)
    : images_(images), board_image_path_(std::move(board_image_path)) {
}

Img UIManager::render(const GameSnapshot& snapshot, int dt_ms, std::optional<Position> selected_cell) {
    const int board_px_w = snapshot.board_width * constants::kCellSizePx;
    const int board_px_h = snapshot.board_height * constants::kCellSizePx;

    if (!resized_board_.has_value()) {
        resized_board_ = images_.get(board_image_path_).clone();
        resized_board_->resize(board_px_w, board_px_h);
    }
    Img frame = resized_board_->clone();

    for (const auto& piece : snapshot.pieces) {
        int cell_x = lerp(piece.pixels_location.x, piece.target_pixels_location.x, piece.progress);
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
        frame.draw_rectangle(selected_cell->x * constants::kCellSizePx, selected_cell->y * constants::kCellSizePx,
                              constants::kCellSizePx, constants::kCellSizePx, cv::Scalar(0, 255, 255, 255), 3);
    }

    if (snapshot.is_game_over) {
        frame.put_text("Game Over", 40, frame.get_mat().rows / 2, 1.5, cv::Scalar(0, 0, 255, 255), 3);
    }

    return frame;
}
