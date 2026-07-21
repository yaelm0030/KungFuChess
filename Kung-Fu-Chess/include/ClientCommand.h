#pragma once

#include <optional>
#include <string>

#include "Types.h"

class Controller;

// Click/jump-only dispatcher for untrusted network input (GameServer's future inbound
// channel). Silently ignores malformed or unknown commands, same policy as
// CommandProcessor, but has no reply channel and no "wait"/"print".
class ClientCommand {
public:
    // acting_color is forwarded to Controller::click/jump; see there for its meaning.
    static void apply(Controller& controller, const std::string& line, std::optional<Color> acting_color = std::nullopt);
};
