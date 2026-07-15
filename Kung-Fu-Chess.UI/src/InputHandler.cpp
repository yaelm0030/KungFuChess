#include "InputHandler.h"

#include <opencv2/opencv.hpp>

InputHandler::InputHandler(Controller& controller, const std::string& window_name)
    : controller_(controller) {
    cv::setMouseCallback(window_name, &InputHandler::on_mouse, this);
}

void InputHandler::on_mouse(int event, int x, int y, int flags, void* userdata) {
    static_cast<InputHandler*>(userdata)->handle_event(event, x, y);
}

void InputHandler::handle_event(int event, int x, int y) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        controller_.click(x, y);
    } else if (event == cv::EVENT_RBUTTONDOWN) {
        controller_.jump(x, y);
    }
}
