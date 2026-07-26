#pragma once

#include <optional>
#include <string>

#include "Types.h"

class Controller;

// Click/jump dispatcher for untrusted network input; unknown commands are ignored.
class ClientCommand {
public:
    // acting_color is forwarded to Controller::click/jump.
    static void apply(Controller& controller, const std::string& line, std::optional<Color> acting_color = std::nullopt);
};
