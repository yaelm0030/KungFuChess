#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Parser.h"

TEST_SUITE("Parser::board_to_string") {

// ---- degenerate board sizes ------------------------------------------

TEST_CASE("a 0x0 board stringifies to an empty string") {
    Board board(0, 0);
    CHECK(Parser::board_to_string(board) == "");
}

TEST_CASE("zero width with rows produces one empty line per row") {
    Board board(0, 3);
    CHECK(Parser::board_to_string(board) == "\n\n");
}

TEST_CASE("zero height with columns produces an empty string") {
    Board board(5, 0);
    CHECK(Parser::board_to_string(board) == "");
}

// ---- single cell ---------------------------------------------------------

TEST_CASE("a 1x1 empty board stringifies to a single dot") {
    Board board(1, 1);
    CHECK(Parser::board_to_string(board) == ".");
}

TEST_CASE("a 1x1 board with a piece stringifies to its token") {
    Board board(1, 1);
    board.place_at(0, 0, Cell{ Color::b, PieceType::N });
    CHECK(Parser::board_to_string(board) == "bN");
}

TEST_CASE("every color/piece combination stringifies to the matching token") {
    struct Case { Color color; PieceType type; std::string token; };
    const Case cases[] = {
        { Color::w, PieceType::K, "wK" }, { Color::w, PieceType::Q, "wQ" },
        { Color::w, PieceType::R, "wR" }, { Color::w, PieceType::B, "wB" },
        { Color::w, PieceType::N, "wN" }, { Color::w, PieceType::P, "wP" },
        { Color::b, PieceType::K, "bK" }, { Color::b, PieceType::Q, "bQ" },
        { Color::b, PieceType::R, "bR" }, { Color::b, PieceType::B, "bB" },
        { Color::b, PieceType::N, "bN" }, { Color::b, PieceType::P, "bP" },
    };
    for (const auto& c : cases) {
        CAPTURE(c.token);
        Board board(1, 1);
        board.place_at(0, 0, Cell{ c.color, c.type });
        CHECK(Parser::board_to_string(board) == c.token);
    }
}

// ---- layout: separators between cells and rows ---------------------------

TEST_CASE("cells within a row are separated by a single space") {
    Board board(3, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(2, 0, Cell{ Color::b, PieceType::R });
    CHECK(Parser::board_to_string(board) == "wR . bR");
}

TEST_CASE("rows are separated by a newline with no trailing newline") {
    Board board(2, 2);
    board.place_at(0, 0, Cell{ Color::w, PieceType::K });
    board.place_at(1, 1, Cell{ Color::b, PieceType::K });
    CHECK(Parser::board_to_string(board) == "wK .\n. bK");
}

TEST_CASE("row-major order is preserved: rows are emitted before columns") {
    Board board(2, 3);
    board.place_at(0, 0, Cell{ Color::w, PieceType::P });
    board.place_at(1, 2, Cell{ Color::b, PieceType::P });
    CHECK(Parser::board_to_string(board) == "wP .\n. .\n. bP");
}

}
