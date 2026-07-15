#include "Board.h"
#include "Controller.h"
#include "GameSnapshot.h"
#include "ImageCache.h"
#include "InputHandler.h"
#include "Parser.h"
#include "Position.h"
#include "UIManager.h"

#include <chrono>
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

// How long each spin blocks pumping GUI/input events for; not the
// simulation's dt, which is measured separately from the real clock below.
constexpr int kPollMs = 1;

} // namespace

int main() {
    ImageCache images;
    UIManager ui(images, "assets/images/board.png");

    Controller controller(starting_board());

    const std::string window_name = "Kung Fu Chess";
    cv::namedWindow(window_name);

    InputHandler input(controller, window_name);

    auto last_tick = std::chrono::steady_clock::now();
    while (!controller.game_over()) {
        auto now = std::chrono::steady_clock::now();
        int dt_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick).count());
        last_tick = now;

        controller.wait(dt_ms);

        Img frame = ui.render(controller.snapshot(), dt_ms, controller.selected());
        cv::imshow(window_name, frame.get_mat());

        if (cv::waitKey(kPollMs) == 27) { // Esc quits
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
