#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "Board.h"

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& code) : std::runtime_error(code) {}
};

class Parser {
public:
    static Board parse_board(const std::vector<std::string>& lines);
    static std::string board_to_string(const Board& board);

    // Splits a line into whitespace-separated tokens; also used by
    // CommandProcessor to tokenize protocol commands.
    static std::vector<std::string> tokenize(const std::string& line);

private:
    static Cell parse_token(const std::string& token);
    static std::string token_from_cell(const std::optional<Cell>& cell);
};
