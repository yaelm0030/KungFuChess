#include "Board.h"
#include "Controller.h"
#include "GameSnapshot.h"
#include "ImageCache.h"
#include "InputHandler.h"
#include "Parser.h"
#include "Position.h"
#include "UIManager.h"

#include <iostream>
#include <optional>
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

} // namespace

int main() {
    ImageCache images;
    UIManager ui(images, "assets/images/board.png");

    Controller controller(starting_board());

    const std::string window_name = "Kung Fu Chess";
    Img frame = ui.render(controller.snapshot());
    cv::namedWindow(window_name);
    cv::imshow(window_name, frame.get_mat());

    InputHandler input(controller, window_name);

    std::optional<Position> last_selected;
    while (cv::waitKey(30) != 27) { // Esc quits
        if (!(controller.selected() == last_selected)) {
            last_selected = controller.selected();
            if (last_selected.has_value()) {
                std::cout << "selected: (" << last_selected->x << ", " << last_selected->y << ")\n";
            } else {
                std::cout << "selection cleared\n";
            }
            frame = ui.render(controller.snapshot(), last_selected);
            cv::imshow(window_name, frame.get_mat());
        }
        if (controller.game_over()) {
            std::cout << "game over\n";
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
