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

TEST_SUITE("GameEngine::promotion") {

TEST_CASE("a white pawn reaching the last row becomes a queen") {
    GameEngine engine(Parser::parse_board({
        ".  .",
        "wP .",
        ".  .",
    }));
    engine.click(50, 150);  // select wP at (0,1)
    engine.click(50, 50);   // move to (0,0), the last row for white

    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == "wQ .\n. .\n. .\n");
}

TEST_CASE("a black pawn reaching the last row becomes a queen") {
    GameEngine engine(Parser::parse_board({
        ".  .",
        "bP .",
        ".  .",
    }));
    engine.click(50, 150);  // select bP at (0,1)
    engine.click(50, 250);  // move to (0,2), the last row for black

    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == ". .\n. .\nbQ .\n");
}

TEST_CASE("a pawn short of the last row does not get promoted") {
    GameEngine engine(Parser::parse_board({
        ".  .",
        ".  .",
        "wP .",
    }));
    engine.click(50, 250);  // select wP at (0,2)
    engine.click(50, 150);  // move to (0,1); not the last row yet

    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(board_of(engine) == ". .\nwP .\n. .\n");
}

TEST_CASE("a promoted pawn capturing the enemy king still ends the game") {
    GameEngine engine(Parser::parse_board({
        ".  bK",
        "wP .",
    }));
    engine.click(50, 150);   // select wP at (0,1)
    engine.click(150, 50);  // diagonal capture onto (1,0), capturing bK and promoting

    engine.wait(GameEngine::kDefaultMoveMsPerCell);
    CHECK(engine.game_over());
    CHECK(board_of(engine) == ". wQ\n. .\n");
}

}
