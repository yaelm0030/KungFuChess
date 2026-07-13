#include "ThirdParty/doctest.h"

#include <sstream>
#include <string>

#include "CommandProcessor.h"
#include "Controller.h"
#include "Parser.h"
#include "ProtocolIO.h"

namespace {

Board make_board() {
    return Parser::parse_board({
        "bR bN .",
        ".  .  .",
        "wR .  wN",
    });
}

} // namespace

TEST_SUITE("ProtocolIO::read_board") {

TEST_CASE("a missing Board: header returns nullopt and writes nothing") {
    std::istringstream in("Wrong:\n");
    std::ostringstream out;
    ProtocolIO io(in, out);

    CHECK_FALSE(io.read_board().has_value());
    CHECK(out.str().empty());
}

TEST_CASE("empty input at EOF returns nullopt and writes nothing") {
    std::istringstream in("");
    std::ostringstream out;
    ProtocolIO io(in, out);

    CHECK_FALSE(io.read_board().has_value());
    CHECK(out.str().empty());
}

TEST_CASE("the Board: header is recognized despite surrounding whitespace") {
    std::istringstream in("  Board:  \nwK\nCommands:\n");
    std::ostringstream out;
    ProtocolIO io(in, out);

    std::optional<Board> board = io.read_board();
    REQUIRE(board.has_value());
    CHECK(Parser::board_to_string(*board) == Parser::board_to_string(Parser::parse_board({ "wK" })));
}

TEST_CASE("a valid board followed by Commands: is parsed the same way as Parser::parse_board") {
    std::istringstream in("Board:\nwK . bK\nCommands:\n");
    std::ostringstream out;
    ProtocolIO io(in, out);

    std::optional<Board> board = io.read_board();
    REQUIRE(board.has_value());
    CHECK(Parser::board_to_string(*board) == Parser::board_to_string(Parser::parse_board({ "wK . bK" })));
}

TEST_CASE("board text that fails to parse returns nullopt and writes the ERROR line") {
    std::istringstream in("Board:\nwX\nCommands:\n");
    std::ostringstream out;
    ProtocolIO io(in, out);

    CHECK_FALSE(io.read_board().has_value());
    CHECK(out.str() == "ERROR UNKNOWN_TOKEN\n");
}

TEST_CASE("board lines with no Commands: marker are all read up to EOF") {
    std::istringstream in("Board:\nwK . bK\n");
    std::ostringstream out;
    ProtocolIO io(in, out);

    std::optional<Board> board = io.read_board();
    REQUIRE(board.has_value());
    CHECK(Parser::board_to_string(*board) == Parser::board_to_string(Parser::parse_board({ "wK . bK" })));
}

}

TEST_SUITE("ProtocolIO::run_commands") {

TEST_CASE("a click line for a real piece selects it") {
    Controller controller(make_board());

    std::istringstream in("click 50 50\n"); // -> cell (0,0), bR
    std::ostringstream out;
    ProtocolIO io(in, out);
    CommandProcessor processor(controller);

    io.run_commands(processor);
    CHECK(controller.has_selection());
}

TEST_CASE("an empty stream leaves the selection untouched") {
    Controller controller(make_board());

    std::istringstream in("");
    std::ostringstream out;
    ProtocolIO io(in, out);
    CommandProcessor processor(controller);

    io.run_commands(processor);
    CHECK_FALSE(controller.has_selection());
}

TEST_CASE("a blank line and an unknown command do not block a later valid click") {
    Controller controller(make_board());

    std::istringstream in("\nfoo bar\nclick 50 50\n");
    std::ostringstream out;
    ProtocolIO io(in, out);
    CommandProcessor processor(controller);

    io.run_commands(processor);
    CHECK(controller.has_selection());
}

}
