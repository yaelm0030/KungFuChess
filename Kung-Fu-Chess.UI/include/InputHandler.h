#pragma once

#include "ServerConnection.h"

#include <string>

class InputHandler {
public:
    InputHandler(ServerConnection& server, const std::string& window_name);

private:
    static void on_mouse(int event, int x, int y, int flags, void* userdata);
    void handle_event(int event, int x, int y);

    ServerConnection& server_;
};
