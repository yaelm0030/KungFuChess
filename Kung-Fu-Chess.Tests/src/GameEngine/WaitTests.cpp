#include "ThirdParty/doctest.h"

#include <sstream>

#include "GameEngine.h"
#include "Parser.h"

TEST_SUITE("GameEngine::wait") {

TEST_CASE("the clock starts at zero") {
    GameEngine engine(Parser::parse_board({ "wK" }));
    CHECK(engine.clock_ms() == 0);
}

TEST_CASE("wait advances the clock by the given number of milliseconds") {
    GameEngine engine(Parser::parse_board({ "wK" }));
    engine.wait(1000);
    CHECK(engine.clock_ms() == 1000);
}

TEST_CASE("repeated waits accumulate") {
    GameEngine engine(Parser::parse_board({ "wK" }));
    engine.wait(1000);
    engine.wait(250);
    engine.wait(1);
    CHECK(engine.clock_ms() == 1251);
}

TEST_CASE("waiting zero milliseconds does not change the clock") {
    GameEngine engine(Parser::parse_board({ "wK" }));
    engine.wait(1000);
    engine.wait(0);
    CHECK(engine.clock_ms() == 1000);
}

TEST_CASE("a negative wait is ignored rather than rewinding the clock") {
    GameEngine engine(Parser::parse_board({ "wK" }));
    engine.wait(1000);
    engine.wait(-500);
    CHECK(engine.clock_ms() == 1000);
}

TEST_CASE("a move does not settle on the board until wait reaches its arrival time") {
    GameEngine engine(Parser::parse_board({ "wK ." }));
    engine.click(50, 50);  // select wK at (0,0)
    engine.click(150, 50); // move to (1,0); one cell of travel time is needed
    CHECK_FALSE(engine.has_selection());

    engine.wait(GameEngine::kDefaultMoveMsPerCell - 1);
    CHECK(engine.clock_ms() == GameEngine::kDefaultMoveMsPerCell - 1);

    std::ostringstream still_at_start;
    engine.print(still_at_start);
    CHECK(still_at_start.str() == "wK .\n");

    engine.wait(1);
    std::ostringstream arrived;
    engine.print(arrived);
    CHECK(arrived.str() == ". wK\n");
}

}
