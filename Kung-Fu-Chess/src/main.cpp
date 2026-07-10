#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Board.h"
#include "GameEngine.h"
#include "Parser.h"

namespace {

std::string trim(const std::string& s) {
    size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

std::vector<std::string> read_lines_until(std::istream& in, const std::string& stop_marker) {
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (trim(line) == stop_marker) {
            break;
        }
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

void run_command(const std::vector<std::string>& tokens, GameEngine& engine) {
    if (tokens.empty()) {
        return;
    }

    try {
        if (tokens[0] == "click" && tokens.size() == 3) {
            engine.click(std::stoi(tokens[1]), std::stoi(tokens[2]));
        } else if (tokens[0] == "jump" && tokens.size() == 3) {
            engine.jump(std::stoi(tokens[1]), std::stoi(tokens[2]));
        } else if (tokens[0] == "wait" && tokens.size() == 2) {
            engine.wait(std::stoi(tokens[1]));
        } else if (tokens[0] == "print" && tokens.size() == 2 && tokens[1] == "board") {
            engine.print();
        }
    } catch (const std::exception&) {
        // Malformed numeric arguments (e.g. "click a b") are ignored.
    }
}

} // namespace

int main() {
    std::string first_line;
    if (!std::getline(std::cin, first_line) || trim(first_line) != "Board:") {
        return 0;
    }

    std::vector<std::string> board_lines = read_lines_until(std::cin, "Commands:");

    Board board(0, 0);
    try {
        board = Parser::parse_board(board_lines);
    } catch (const ParseError& e) {
        std::cout << "ERROR " << e.what() << "\n";
        return 0;
    }

    GameEngine engine(std::move(board));

    std::string command;
    while (std::getline(std::cin, command)) {
        run_command(tokenize(command), engine);
    }
}
