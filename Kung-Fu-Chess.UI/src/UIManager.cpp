#include "UIManager.h"

#include <cmath>
#include <stdexcept>

namespace {

char piece_type_letter(PieceType type) {
    switch (type) {
        case PieceType::K: return 'K';
        case PieceType::Q: return 'Q';
        case PieceType::R: return 'R';
        case PieceType::B: return 'B';
        case PieceType::N: return 'N';
        case PieceType::P: return 'P';
    }
    throw std::invalid_argument("Unknown PieceType");
}

char color_letter(Color color) {
    return color == Color::w ? 'W' : 'B';
}

std::string state_folder(PieceState state) {
    switch (state) {
        case PieceState::idle: return "idle";
        case PieceState::move: return "move";
        case PieceState::jump: return "jump";
        case PieceState::short_rest: return "short_rest";
        case PieceState::long_rest: return "long_rest";
    }
    throw std::invalid_argument("Unknown PieceState");
}

std::string sprite_path(const PieceSnapshot& piece) {
    std::string folder = std::string(1, piece_type_letter(piece.type)) + color_letter(piece.color);
    return "assets/images/pieces/" + folder + "/states/" + state_folder(piece.state) + "/sprites/1.png";
}

} // namespace

UIManager::UIManager(ImageCache& images, std::string board_image_path)
    : images_(images), board_image_path_(std::move(board_image_path)) {
}

Img UIManager::render(const GameSnapshot& snapshot) {
    Img& board = images_.get(board_image_path_);
    Img frame = board.clone();

    int cell_w = static_cast<int>(std::round(static_cast<double>(board.get_mat().cols) / snapshot.board_width));
    int cell_h = static_cast<int>(std::round(static_cast<double>(board.get_mat().rows) / snapshot.board_height));

    for (const auto& piece : snapshot.pieces) {
        Img sprite = images_.get(sprite_path(piece)).clone();
        sprite.resize(cell_w, cell_h);
        sprite.draw_on(frame, piece.pixels_location.x, piece.pixels_location.y);
    }

    if (snapshot.is_game_over) {
        frame.put_text("Game Over", 40, frame.get_mat().rows / 2, 1.5, cv::Scalar(0, 0, 255, 255), 3);
    }

    return frame;
}
