#include "InputHandler.h"

#include "UIConstants.h"

#include <unordered_map>

#include <opencv2/opencv.hpp>

namespace {

// Maps an OpenCV mouse event to the wire verb ClientCommand::apply understands.
const std::unordered_map<int, std::string>& event_verbs() {
    static const std::unordered_map<int, std::string> verbs = {
        { cv::EVENT_LBUTTONDOWN, "click" },
        { cv::EVENT_RBUTTONDOWN, "jump" },
    };
    return verbs;
}

} // namespace

InputHandler::InputHandler(ServerConnection& server, const std::string& window_name)
    : server_(server) {
    cv::setMouseCallback(window_name, &InputHandler::on_mouse, this);
}

void InputHandler::on_mouse(int event, int x, int y, int flags, void* userdata) {
    static_cast<InputHandler*>(userdata)->handle_event(event, x, y);
}

void InputHandler::handle_event(int event, int x, int y) {
    const auto& verbs = event_verbs();
    auto it = verbs.find(event);
    if (it == verbs.end()) {
        return;
    }

    // A panel click becomes a negative board x, already treated as outside the board.
    int board_x = x - ui_constants::kBoardOffsetX;
    server_.send(it->second + " " + std::to_string(board_x) + " " + std::to_string(y));
}
