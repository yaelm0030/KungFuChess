#include "Parser.h"

#include <sstream>

// Splits a line into whitespace-separated tokens.
std::vector<std::string> Parser::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Parses a two-character token, such as "wK", into a Cell.
Cell Parser::parse_token(const std::string& token) {
    if (token.size() != 2) {
        throw ParseError("UNKNOWN_TOKEN");
    }

    Color color;
    switch (token[0]) {
        case 'w': color = Color::w; break;
        case 'b': color = Color::b; break;
        default: throw ParseError("UNKNOWN_TOKEN");
    }

    PieceType type;
    switch (token[1]) {
        case 'K': type = PieceType::K; break;
        case 'Q': type = PieceType::Q; break;
        case 'R': type = PieceType::R; break;
        case 'B': type = PieceType::B; break;
        case 'N': type = PieceType::N; break;
        case 'P': type = PieceType::P; break;
        default: throw ParseError("UNKNOWN_TOKEN");
    }

    return Cell{ color, type };
}

// Renders a cell back into its two-character token, or "." if empty.
std::string Parser::token_from_cell(const std::optional<Cell>& cell) {
    if (!cell.has_value()) {
        return ".";
    }

    std::string token;
    token += (cell->color == Color::w) ? 'w' : 'b';

    switch (cell->type) {
        case PieceType::K: token += 'K'; break;
        case PieceType::Q: token += 'Q'; break;
        case PieceType::R: token += 'R'; break;
        case PieceType::B: token += 'B'; break;
        case PieceType::N: token += 'N'; break;
        case PieceType::P: token += 'P'; break;
    }

    return token;
}

// Parses board text into a Board: blank lines are skipped, and every
// non-blank row must tokenize to the same width as the first one.
Board Parser::parse_board(const std::vector<std::string>& lines) {
    std::vector<std::vector<std::string>> rows;
    size_t width = 0;

    for (const auto& line : lines) {
        std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty()) {
            continue;
        }

        if (rows.empty()) {
            width = tokens.size();
        } else if (tokens.size() != width) {
            throw ParseError("ROW_WIDTH_MISMATCH");
        }

        rows.push_back(std::move(tokens));
    }

    Board board(static_cast<int>(width), static_cast<int>(rows.size()));

    for (size_t y = 0; y < rows.size(); ++y) {
        for (size_t x = 0; x < rows[y].size(); ++x) {
            const std::string& token = rows[y][x];
            if (token == ".") {
                continue;
            }
            board.place_at(static_cast<int>(x), static_cast<int>(y), parse_token(token));
        }
    }

    return board;
}

// Renders a board back into its space-separated, newline-delimited text form.
std::string Parser::board_to_string(const Board& board) {
    std::ostringstream oss;
    for (int y = 0; y < board.get_height(); ++y) {
        if (y > 0) {
            oss << '\n';
        }
        for (int x = 0; x < board.get_width(); ++x) {
            if (x > 0) {
                oss << ' ';
            }
            oss << token_from_cell(board.get_at(x, y));
        }
    }
    return oss.str();
}
