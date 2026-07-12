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
    CHECK(engine.request_move(Position{ 0, 0 }, Position{ 1, 0 })); // one cell of travel time is needed

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

TEST_CASE("a piece with a pending move cannot be given a second, different destination") {
    // Today the rejection comes from conflicts_with_pending_move (the two routes
    // share the start cell); GameEngine's own is_moving check is a defensive
    // invariant in case that incidental route-sharing behavior ever changes.
    GameEngine engine(Parser::parse_board({ "wR . ." }));
    CHECK(engine.request_move(Position{ 0, 0 }, Position{ 1, 0 })); // scheduled; not yet arrived
    CHECK_FALSE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 })); // redirect rejected
}

}
