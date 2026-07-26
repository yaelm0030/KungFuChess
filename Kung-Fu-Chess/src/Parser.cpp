#include "Parser.h"

#include <sstream>
#include <unordered_map>

namespace {

const std::unordered_map<char, PieceType> kPieceTypesByLetter{
    { 'K', PieceType::K }, { 'Q', PieceType::Q }, { 'R', PieceType::R },
    { 'B', PieceType::B }, { 'N', PieceType::N }, { 'P', PieceType::P },
};

std::unordered_map<PieceType, char> invert_piece_letters() {
    std::unordered_map<PieceType, char> letters;
    for (const auto& [letter, type] : kPieceTypesByLetter) {
        letters[type] = letter;
    }
    return letters;
}

const std::unordered_map<PieceType, char> kPieceTypeLetters = invert_piece_letters();

} // namespace

std::vector<std::string> Parser::tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

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

    auto it = kPieceTypesByLetter.find(token[1]);
    if (it == kPieceTypesByLetter.end()) {
        throw ParseError("UNKNOWN_TOKEN");
    }

    return Cell{ color, it->second };
}

std::string Parser::token_from_cell(const std::optional<Cell>& cell) {
    if (!cell.has_value()) {
        return ".";
    }

    std::string token;
    token += (cell->color == Color::w) ? 'w' : 'b';
    token += kPieceTypeLetters.at(cell->type);

    return token;
}

// Blank lines are skipped; every row must match the first row's width.
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
