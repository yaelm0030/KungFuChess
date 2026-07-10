#include "ThirdParty/doctest.h"

#include <sstream>
#include <string>
#include <vector>

#include "Board.h"
#include "Parser.h"

namespace {

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    return lines;
}

} // namespace

TEST_SUITE("Parser round trip") {

TEST_CASE("parse then stringify normalizes whitespace to a single space") {
    Board board = Parser::parse_board({ "wK   .    bK" });
    CHECK(Parser::board_to_string(board) == "wK . bK");
}

TEST_CASE("parse then stringify drops blank lines from the output") {
    Board board = Parser::parse_board({ "wK", "", "bK" });
    CHECK(Parser::board_to_string(board) == "wK\nbK");
}

TEST_CASE("stringify then parse reproduces an equivalent board") {
    Board original(3, 2);
    original.place_at(0, 0, Cell{ Color::w, PieceType::R });
    original.place_at(2, 1, Cell{ Color::b, PieceType::Q });

    Board round_tripped = Parser::parse_board(split_lines(Parser::board_to_string(original)));

    CHECK(round_tripped.get_width() == original.get_width());
    CHECK(round_tripped.get_height() == original.get_height());

    for (int y = 0; y < original.get_height(); ++y) {
        for (int x = 0; x < original.get_width(); ++x) {
            CAPTURE(x);
            CAPTURE(y);
            auto expected = original.get_at(x, y);
            auto actual = round_tripped.get_at(x, y);
            REQUIRE(expected.has_value() == actual.has_value());
            if (expected.has_value()) {
                CHECK(expected->color == actual->color);
                CHECK(expected->type == actual->type);
            }
        }
    }
}

TEST_CASE("stringify then parse is idempotent for an already-normalized board") {
    Board board(4, 1);
    board.place_at(1, 0, Cell{ Color::w, PieceType::B });
    board.place_at(3, 0, Cell{ Color::b, PieceType::N });

    std::string first = Parser::board_to_string(board);
    std::string second = Parser::board_to_string(Parser::parse_board(split_lines(first)));

    CHECK(first == second);
}

}
