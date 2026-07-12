#include <iostream>
#include <string>
#include <vector>

#include "Board.h"
#include "CommandProcessor.h"
#include "Controller.h"
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

    Controller controller(std::move(board));
    CommandProcessor processor(controller);

    std::string command;
    while (std::getline(std::cin, command)) {
        processor.run_line(command);
    }
}
