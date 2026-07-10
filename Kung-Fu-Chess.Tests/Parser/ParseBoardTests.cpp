#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Parser.h"

TEST_SUITE("Parser::parse_board") {

// ---- empty / blank input --------------------------------------------

TEST_CASE("no lines at all produces an empty 0x0 board") {
    Board board = Parser::parse_board({});
    CHECK(board.get_width() == 0);
    CHECK(board.get_height() == 0);
}

TEST_CASE("blank lines alone produce an empty 0x0 board") {
    SUBCASE("single empty line") {
        Board board = Parser::parse_board({ "" });
        CHECK(board.get_width() == 0);
        CHECK(board.get_height() == 0);
    }
    SUBCASE("whitespace-only line") {
        Board board = Parser::parse_board({ "   \t  " });
        CHECK(board.get_width() == 0);
        CHECK(board.get_height() == 0);
    }
    SUBCASE("several blank lines") {
        Board board = Parser::parse_board({ "", "   ", "\t" });
        CHECK(board.get_width() == 0);
        CHECK(board.get_height() == 0);
    }
}

TEST_CASE("blank lines interspersed with rows are skipped, not kept as empty rows") {
    Board board = Parser::parse_board({ "wK", "", "bK" });
    CHECK(board.get_width() == 1);
    CHECK(board.get_height() == 2);

    REQUIRE(board.get_at(0, 0).has_value());
    CHECK(board.get_at(0, 0)->color == Color::w);
    CHECK(board.get_at(0, 0)->type == PieceType::K);

    REQUIRE(board.get_at(0, 1).has_value());
    CHECK(board.get_at(0, 1)->color == Color::b);
    CHECK(board.get_at(0, 1)->type == PieceType::K);
}

TEST_CASE("the board width is fixed by the first non-blank row") {
    Board board = Parser::parse_board({ "", "wK . bK" });
    CHECK(board.get_width() == 3);
    CHECK(board.get_height() == 1);
}

// ---- single cells ------------------------------------------------------

TEST_CASE("a single '.' token produces a 1x1 board with an empty cell") {
    Board board = Parser::parse_board({ "." });
    CHECK(board.get_width() == 1);
    CHECK(board.get_height() == 1);
    CHECK_FALSE(board.get_at(0, 0).has_value());
}

TEST_CASE("a single valid piece token produces a 1x1 board with that piece") {
    Board board = Parser::parse_board({ "wK" });
    CHECK(board.get_width() == 1);
    CHECK(board.get_height() == 1);
    REQUIRE(board.get_at(0, 0).has_value());
    CHECK(board.get_at(0, 0)->color == Color::w);
    CHECK(board.get_at(0, 0)->type == PieceType::K);
}

TEST_CASE("every color/piece token combination parses to the matching cell") {
    struct Case { std::string token; Color color; PieceType type; };
    const Case cases[] = {
        { "wK", Color::w, PieceType::K }, { "wQ", Color::w, PieceType::Q },
        { "wR", Color::w, PieceType::R }, { "wB", Color::w, PieceType::B },
        { "wN", Color::w, PieceType::N }, { "wP", Color::w, PieceType::P },
        { "bK", Color::b, PieceType::K }, { "bQ", Color::b, PieceType::Q },
        { "bR", Color::b, PieceType::R }, { "bB", Color::b, PieceType::B },
        { "bN", Color::b, PieceType::N }, { "bP", Color::b, PieceType::P },
    };
    for (const auto& c : cases) {
        CAPTURE(c.token);
        Board board = Parser::parse_board({ c.token });
        REQUIRE(board.get_at(0, 0).has_value());
        CHECK(board.get_at(0, 0)->color == c.color);
        CHECK(board.get_at(0, 0)->type == c.type);
    }
}

// ---- tokenizing / whitespace handling -----------------------------------

TEST_CASE("tokens may be separated by arbitrary runs of whitespace") {
    SUBCASE("multiple spaces between tokens") {
        Board board = Parser::parse_board({ "wK    .    bK" });
        CHECK(board.get_width() == 3);
    }
    SUBCASE("tabs between tokens") {
        Board board = Parser::parse_board({ "wK\t.\tbK" });
        CHECK(board.get_width() == 3);
    }
    SUBCASE("leading and trailing whitespace on the line") {
        Board board = Parser::parse_board({ "   wK . bK   " });
        CHECK(board.get_width() == 3);
        CHECK(board.get_at(0, 0).has_value());
        CHECK_FALSE(board.get_at(1, 0).has_value());
        CHECK(board.get_at(2, 0).has_value());
    }
}

// ---- board shape ---------------------------------------------------------

TEST_CASE("rectangular boards keep row-major x/y placement") {
    Board board = Parser::parse_board({
        "wR wN wB",
        "bR bN bB",
    });
    CHECK(board.get_width() == 3);
    CHECK(board.get_height() == 2);

    REQUIRE(board.get_at(0, 0).has_value());
    CHECK(board.get_at(0, 0)->color == Color::w);
    CHECK(board.get_at(0, 0)->type == PieceType::R);

    REQUIRE(board.get_at(2, 1).has_value());
    CHECK(board.get_at(2, 1)->color == Color::b);
    CHECK(board.get_at(2, 1)->type == PieceType::B);
}

TEST_CASE("a single column with multiple rows parses correctly") {
    Board board = Parser::parse_board({ "wP", ".", "bP" });
    CHECK(board.get_width() == 1);
    CHECK(board.get_height() == 3);
    REQUIRE(board.get_at(0, 0).has_value());
    CHECK_FALSE(board.get_at(0, 1).has_value());
    REQUIRE(board.get_at(0, 2).has_value());
}

TEST_CASE("a wide row with mixed pieces and empty cells parses correctly") {
    Board board = Parser::parse_board({ "wR wN wB wQ wK wB wN wR" });
    CHECK(board.get_width() == 8);
    for (int x = 0; x < 8; ++x) {
        REQUIRE(board.get_at(x, 0).has_value());
        CHECK(board.get_at(x, 0)->color == Color::w);
    }
}

// ---- ROW_WIDTH_MISMATCH ---------------------------------------------------

TEST_CASE("a second row shorter than the first throws ROW_WIDTH_MISMATCH") {
    CHECK_THROWS_WITH_AS(
        Parser::parse_board({ "wK . bK", "wK ." }),
        "ROW_WIDTH_MISMATCH",
        ParseError);
}

TEST_CASE("a second row longer than the first throws ROW_WIDTH_MISMATCH") {
    CHECK_THROWS_WITH_AS(
        Parser::parse_board({ "wK .", "wK . bK" }),
        "ROW_WIDTH_MISMATCH",
        ParseError);
}

// ---- UNKNOWN_TOKEN ---------------------------------------------------------

TEST_CASE("a one-character token is rejected") {
    CHECK_THROWS_WITH_AS(Parser::parse_board({ "w" }), "UNKNOWN_TOKEN", ParseError);
}

TEST_CASE("a three-character token is rejected") {
    CHECK_THROWS_WITH_AS(Parser::parse_board({ "wKK" }), "UNKNOWN_TOKEN", ParseError);
}

TEST_CASE("an unknown color letter is rejected") {
    CHECK_THROWS_WITH_AS(Parser::parse_board({ "xK" }), "UNKNOWN_TOKEN", ParseError);
}

TEST_CASE("an unknown piece letter is rejected") {
    CHECK_THROWS_WITH_AS(Parser::parse_board({ "wX" }), "UNKNOWN_TOKEN", ParseError);
}

TEST_CASE("token letters are case sensitive") {
    SUBCASE("lowercase piece letter") {
        CHECK_THROWS_WITH_AS(Parser::parse_board({ "wk" }), "UNKNOWN_TOKEN", ParseError);
    }
    SUBCASE("uppercase color letter") {
        CHECK_THROWS_WITH_AS(Parser::parse_board({ "WK" }), "UNKNOWN_TOKEN", ParseError);
    }
}

TEST_CASE("an invalid token in a later row is still caught") {
    CHECK_THROWS_WITH_AS(
        Parser::parse_board({ "wK . bK", ". zz ." }),
        "UNKNOWN_TOKEN",
        ParseError);
}

}
