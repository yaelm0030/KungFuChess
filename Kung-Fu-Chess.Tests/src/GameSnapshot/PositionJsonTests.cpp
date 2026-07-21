#include "ThirdParty/doctest.h"

#include "GameSnapshotJson.h"

using nlohmann::json;

TEST_SUITE("Position::to_json") {

TEST_CASE("a position serializes to an x/y object") {
    json actual = Position{ 3, 5 };

    CHECK(actual == json{ { "x", 3 }, { "y", 5 } });
}

} // TEST_SUITE

TEST_SUITE("Position::from_json") {

TEST_CASE("an x/y object deserializes to a position") {
    Position actual = json{ { "x", 3 }, { "y", 5 } }.get<Position>();

    CHECK(actual == Position{ 3, 5 });
}

TEST_CASE("a missing y field throws out_of_range") {
    json input = json{ { "x", 3 } };

    CHECK_THROWS_AS(input.get<Position>(), nlohmann::json::out_of_range);
}

} // TEST_SUITE
