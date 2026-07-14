#include "UIManager.h"

#include "Constants.h"

#include <unordered_map>

namespace {

const std::unordered_map<PieceType, char> kPieceTypeLetters{
    { PieceType::K, 'K' },
    { PieceType::Q, 'Q' },
    { PieceType::R, 'R' },
    { PieceType::B, 'B' },
    { PieceType::N, 'N' },
    { PieceType::P, 'P' },
};

const std::unordered_map<PieceState, std::string> kStateFolders{
    { PieceState::idle, "idle" },
    { PieceState::move, "move" },
    { PieceState::jump, "jump" },
    { PieceState::short_rest, "short_rest" },
    { PieceState::long_rest, "long_rest" },
};

char color_letter(Color color) {
    return color == Color::w ? 'W' : 'B';
}

std::string sprite_path(const PieceSnapshot& piece) {
    std::string folder = std::string(1, kPieceTypeLetters.at(piece.type)) + color_letter(piece.color);
    return "assets/images/pieces/" + folder + "/states/" + kStateFolders.at(piece.state) + "/sprites/1.png";
}

} // namespace

UIManager::UIManager(ImageCache& images, std::string board_image_path)
    : images_(images), board_image_path_(std::move(board_image_path)) {
}

Img UIManager::render(const GameSnapshot& snapshot) {
    const int board_px_w = snapshot.board_width * constants::kCellSizePx;
    const int board_px_h = snapshot.board_height * constants::kCellSizePx;

    Img frame = images_.get(board_image_path_).clone();
    frame.resize(board_px_w, board_px_h);

    for (const auto& piece : snapshot.pieces) {
        Img sprite = images_.get(sprite_path(piece)).clone();
        sprite.resize(constants::kCellSizePx, constants::kCellSizePx);
        sprite.draw_on(frame, piece.pixels_location.x, piece.pixels_location.y);
    }

    if (snapshot.is_game_over) {
        frame.put_text("Game Over", 40, frame.get_mat().rows / 2, 1.5, cv::Scalar(0, 0, 255, 255), 3);
    }

    return frame;
}
