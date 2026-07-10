#include <iostream>
#include <string>
#include <vector>

#include "Board.h"
#include "Parser.h"

namespace {

std::vector<std::string> read_lines_until(std::istream& in, const std::string& stop_marker) {
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (line == stop_marker) {
            break;
        }
        lines.push_back(line);
    }
    return lines;
}

void run_command(const std::string& command, const Board& board) {
    if (command == "print board") {
        std::cout << Parser::board_to_string(board) << "\n";
    }
}

} // namespace

int main() {
    std::string first_line;
    if (!std::getline(std::cin, first_line) || first_line != "Board:") {
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

    std::string command;
    while (std::getline(std::cin, command)) {
        run_command(command, board);
    }
}
