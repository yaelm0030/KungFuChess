#include "GameSnapshot.h"
#include "ImageCache.h"
#include "UIManager.h"

#include <cmath>

namespace {

PixelPosition cell_pixel(int col, int row) {
    const double cell_w = 822.0 / 8;
    const double cell_h = 828.0 / 8;
    return { static_cast<int>(std::round(col * cell_w)), static_cast<int>(std::round(row * cell_h)) };
}

} // namespace

int main() {
    ImageCache images;
    UIManager ui(images, "assets/images/board.png");

    GameSnapshot snapshot;
    snapshot.board_width = 8;
    snapshot.board_height = 8;
    snapshot.is_game_over = false;
    snapshot.pieces = {
        { PieceType::P, Color::w, cell_pixel(0, 6), PieceState::idle },
        { PieceType::K, Color::b, cell_pixel(4, 0), PieceState::idle },
        { PieceType::N, Color::w, cell_pixel(1, 3), PieceState::move },
    };

    Img frame = ui.render(snapshot);
    frame.show();
    return 0;
}
