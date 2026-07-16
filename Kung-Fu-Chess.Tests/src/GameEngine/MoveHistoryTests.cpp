#include "ThirdParty/doctest.h"

#include "GameEngine.h"
#include "Parser.h"

namespace {

Board make_board() {
    return Parser::parse_board({
        "bR bN .",
        ".  .  .",
        "wR .  wN",
    });
}

} // namespace

TEST_SUITE("GameEngine::move_history") {

TEST_CASE("history starts empty") {
    GameEngine engine(make_board());
    CHECK(engine.move_history().empty());
}

TEST_CASE("a successful move appends a record with the piece's type, color, source, and destination") {
    GameEngine engine(make_board());
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 0, 1 })); // bR down one cell

    REQUIRE(engine.move_history().size() == 1);
    const MoveRecord& record = engine.move_history()[0];
    CHECK(record.type == PieceType::R);
    CHECK(record.color == Color::b);
    CHECK(record.source == Position{ 0, 0 });
    CHECK(record.destination == Position{ 0, 1 });
}

TEST_CASE("an illegal move is not appended") {
    GameEngine engine(make_board());
    CHECK_FALSE(engine.request_move(Position{ 0, 0 }, Position{ 1, 1 })); // diagonal; illegal for a rook

    CHECK(engine.move_history().empty());
}

TEST_CASE("a jump is not appended") {
    GameEngine engine(make_board());
    REQUIRE(engine.request_jump(Position{ 0, 0 }));

    CHECK(engine.move_history().empty());
}

TEST_CASE("multiple successful moves are appended in order") {
    GameEngine engine(make_board());
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 0, 1 })); // bR down one cell
    REQUIRE(engine.request_move(Position{ 2, 2 }, Position{ 0, 1 })); // wN's L-shaped move; (0,1) is still empty on the board until bR's move settles

    REQUIRE(engine.move_history().size() == 2);
    CHECK(engine.move_history()[0].source == Position{ 0, 0 });
    CHECK(engine.move_history()[1].source == Position{ 2, 2 });
}

}
