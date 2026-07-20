#include "ClientCommand.h"

#include <vector>

#include "Controller.h"
#include "Parser.h"

void ClientCommand::apply(Controller& controller, const std::string& line) {
    std::vector<std::string> tokens = Parser::tokenize(line);
    if (tokens.empty()) {
        return;
    }

    try {
        if (tokens[0] == "click" && tokens.size() == 3) {
            controller.click(std::stoi(tokens[1]), std::stoi(tokens[2]));
        } else if (tokens[0] == "jump" && tokens.size() == 3) {
            controller.jump(std::stoi(tokens[1]), std::stoi(tokens[2]));
        }
    } catch (const std::exception&) {
        // Malformed numeric arguments (e.g. "click a b") are ignored.
    }
}
