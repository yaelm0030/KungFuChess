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

TEST_SUITE("GameEngine::game_over") {

TEST_CASE("the game is not over before any king is captured") {
    GameEngine engine(Parser::parse_board({ "wR . bK" }));
    CHECK_FALSE(engine.game_over());
}

TEST_CASE("capturing the enemy king ends the game once the move settles") {
    GameEngine engine(Parser::parse_board({ "wR . bK" }));
    engine.click(50, 50);   // select wR at (0,0)
    engine.click(250, 50);  // move across to (2,0), capturing bK; 2 cells of travel time

    CHECK_FALSE(engine.game_over()); // not settled yet
    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK(engine.game_over());
    CHECK(board_of(engine) == ". . wR\n");
}

TEST_CASE("capturing a non-king piece does not end the game") {
    GameEngine engine(Parser::parse_board({ "wR . bR" }));
    engine.click(50, 50);   // select wR at (0,0)
    engine.click(250, 50);  // move across to (2,0), capturing bR

    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK_FALSE(engine.game_over());
}

TEST_CASE("once the game is over, further clicks are ignored") {
    GameEngine engine(Parser::parse_board({ "wR . bK", "wN . ." }));
    engine.click(50, 50);   // select wR at (0,0)
    engine.click(250, 50);  // move across to (2,0), capturing bK
    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    REQUIRE(engine.game_over());

    engine.click(50, 150); // attempt to select wN at (0,1); should be ignored
    CHECK_FALSE(engine.has_selection());
    CHECK(board_of(engine) == ". . wR\nwN . .\n");
}

TEST_CASE("a move already in flight when the game ends still settles on the board") {
    GameEngine engine(Parser::parse_board({
        "wR .  bK .",
        ".  .  .  .",
        "wN .  .  .",
        ".  .  .  .",
    }));
    engine.click(50, 50);   // select wR at (0,0)
    engine.click(250, 50);  // move wR to (2,0), capturing bK; 2 cells of travel time

    engine.click(50, 250);  // select wN at (0,2) before the king capture settles
    engine.click(250, 350); // schedule wN's move to (2,3); an L-shape, also 2 cells of travel time

    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);
    CHECK(engine.game_over());
    CHECK(board_of(engine) == ". . wR .\n. . . .\n. . . .\n. . wN .\n");
}

}
