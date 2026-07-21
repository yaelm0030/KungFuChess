#include "ThirdParty/doctest.h"

#include "GameSnapshotJson.h"

using nlohmann::json;

TEST_SUITE("MoveRecord::to_json") {

TEST_CASE("a move record serializes its type, color, source, and destination") {
    MoveRecord move{ PieceType::R, Color::w, { 0, 0 }, { 0, 2 } };

    json actual = move;

    CHECK(actual == json{
        { "type", "R" },
        { "color", "w" },
        { "source", { { "x", 0 }, { "y", 0 } } },
        { "destination", { { "x", 0 }, { "y", 2 } } },
    });
}

} // TEST_SUITE

TEST_SUITE("MoveRecord::from_json") {

TEST_CASE("type, color, source, and destination round-trip through a move record") {
    json input = json{
        { "type", "R" },
        { "color", "w" },
        { "source", { { "x", 0 }, { "y", 0 } } },
        { "destination", { { "x", 0 }, { "y", 2 } } },
    };

    MoveRecord parsed = input.get<MoveRecord>();

    CHECK(json(parsed) == input);
}

} // TEST_SUITE
