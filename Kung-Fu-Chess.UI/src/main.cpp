#include "Board.h"
#include "Controller.h"
#include "GameSnapshot.h"
#include "ImageCache.h"
#include "InputHandler.h"
#include "Parser.h"
#include "Position.h"
#include "UIManager.h"

#include <opencv2/opencv.hpp>

namespace {

Board starting_board() {
    return Parser::parse_board({
        "bR bN bB bQ bK bB bN bR",
        "bP bP bP bP bP bP bP bP",
        ". . . . . . . .",
        ". . . . . . . .",
        ". . . . . . . .",
        ". . . . . . . .",
        "wP wP wP wP wP wP wP wP",
        "wR wN wB wQ wK wB wN wR",
    });
}

constexpr int kFrameMs = 30;

} // namespace

int main() {
    ImageCache images;
    UIManager ui(images, "assets/images/board.png");

    Controller controller(starting_board());

    const std::string window_name = "Kung Fu Chess";
    cv::namedWindow(window_name);

    InputHandler input(controller, window_name);

    while (!controller.game_over()) {
        controller.wait(kFrameMs);

        Img frame = ui.render(controller.snapshot(), controller.selected());
        cv::imshow(window_name, frame.get_mat());

        if (cv::waitKey(kFrameMs) == 27) { // Esc quits
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
