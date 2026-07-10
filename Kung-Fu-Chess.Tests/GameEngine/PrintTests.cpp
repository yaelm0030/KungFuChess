#include "ThirdParty/doctest.h"

#include <sstream>

#include "GameEngine.h"
#include "Parser.h"

namespace {

std::string board_of(const GameEngine& engine) {
    std::ostringstream oss;
    engine.print(oss);
    return oss.str();
}

} // namespace

TEST_SUITE("GameEngine::print") {

TEST_CASE("printing the initial board matches the parsed board text, plus a trailing newline") {
    std::vector<std::string> lines = { "wR wN", "bR bN" };
    Board board = Parser::parse_board(lines);
    GameEngine engine(Parser::parse_board(lines));

    CHECK(board_of(engine) == Parser::board_to_string(board) + "\n");
}

TEST_CASE("printing a 0x0 board produces just a trailing newline") {
    GameEngine engine(Parser::parse_board({}));
    CHECK(board_of(engine) == "\n");
}

TEST_CASE("print reflects the settled state after a completed move") {
    GameEngine engine(Parser::parse_board({ "wK ." }));
    engine.click(50, 50);  // select wK at (0,0)
    engine.click(150, 50); // move to (1,0)

    CHECK(board_of(engine) == ". wK\n");
}

TEST_CASE("print is unaffected by an in-progress selection with no move yet") {
    GameEngine engine(Parser::parse_board({ "wK ." }));
    std::string before = board_of(engine);

    engine.click(50, 50); // select wK, no move requested

    CHECK(board_of(engine) == before);
}

TEST_CASE("print does not add extra blank lines for boards with empty rows removed during parsing") {
    Board board = Parser::parse_board({ "wK", "", "bK" });
    GameEngine engine(Parser::parse_board({ "wK", "", "bK" }));

    CHECK(board_of(engine) == Parser::board_to_string(board) + "\n");
    CHECK(board_of(engine) == "wK\nbK\n");
}

}
