#include "GameSnapshot.h"
#include "ImageCache.h"
#include "InputHandler.h"
#include "ServerConnection.h"
#include "UIManager.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <opencv2/opencv.hpp>

namespace {

constexpr const char* kServerHost = "localhost";
constexpr uint16_t kServerPort = 9002;

// How long each spin blocks pumping GUI/input events for; not the
// simulation's dt, which is measured separately from the real clock below.
constexpr int kPollMs = 1;

} // namespace

int main() {
    std::cout << "Enter username: ";
    std::string username;
    std::getline(std::cin, username);

    ImageCache images;
    UIManager ui(images, "assets/images/board.png");

    ServerConnection server(kServerHost, kServerPort, username);

    const std::string window_name = "Kung Fu Chess - " + username;
    cv::namedWindow(window_name);

    InputHandler input(server, window_name);

    auto last_tick = std::chrono::steady_clock::now();
    for (;;) {
        auto now = std::chrono::steady_clock::now();
        int dt_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick).count());
        last_tick = now;

        std::optional<GameSnapshot> snapshot = server.latest_snapshot();
        bool game_over = false;
        if (snapshot.has_value()) {
            Img frame = ui.render(*snapshot, dt_ms);
            cv::imshow(window_name, frame.get_mat());
            game_over = snapshot->is_game_over;
        }

        if (cv::waitKey(kPollMs) == 27 || game_over) { // Esc or game over quits
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
