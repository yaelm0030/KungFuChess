#include "ProtocolIO.h"

#include <string>
#include <vector>

#include "CommandProcessor.h"
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

ProtocolIO::ProtocolIO(std::istream& in, std::ostream& out) : in_(in), out_(out) {}

std::optional<Board> ProtocolIO::read_board() {
    std::string first_line;
    if (!std::getline(in_, first_line) || trim(first_line) != "Board:") {
        return std::nullopt;
    }

    std::vector<std::string> board_lines = read_lines_until(in_, "Commands:");

    try {
        return Parser::parse_board(board_lines);
    } catch (const ParseError& e) {
        out_ << "ERROR " << e.what() << "\n";
        return std::nullopt;
    }
}

void ProtocolIO::run_commands(CommandProcessor& processor) {
    std::string command;
    while (std::getline(in_, command)) {
        processor.run_line(command);
    }
}
