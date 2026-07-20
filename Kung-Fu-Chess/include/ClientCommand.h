#pragma once

#include <string>

class Controller;

// Click/jump-only dispatcher for untrusted network input (GameServer's future inbound
// channel). Silently ignores malformed or unknown commands, same policy as
// CommandProcessor, but has no reply channel and no "wait"/"print".
class ClientCommand {
public:
    static void apply(Controller& controller, const std::string& line);
};
