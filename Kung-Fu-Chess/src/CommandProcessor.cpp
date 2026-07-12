#include "CommandProcessor.h"

#include <sstream>
#include <vector>

#include "Controller.h"

namespace {

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

} // namespace

CommandProcessor::CommandProcessor(Controller& controller) : controller_(controller) {}

void CommandProcessor::run_line(const std::string& line) {
    std::vector<std::string> tokens = tokenize(line);
    if (tokens.empty()) {
        return;
    }

    try {
        if (tokens[0] == "click" && tokens.size() == 3) {
            controller_.click(std::stoi(tokens[1]), std::stoi(tokens[2]));
        } else if (tokens[0] == "jump" && tokens.size() == 3) {
            controller_.jump(std::stoi(tokens[1]), std::stoi(tokens[2]));
        } else if (tokens[0] == "wait" && tokens.size() == 2) {
            controller_.wait(std::stoi(tokens[1]));
        } else if (tokens[0] == "print" && tokens.size() == 2 && tokens[1] == "board") {
            controller_.print();
        }
    } catch (const std::exception&) {
        // Malformed numeric arguments (e.g. "click a b") are ignored.
    }
}
