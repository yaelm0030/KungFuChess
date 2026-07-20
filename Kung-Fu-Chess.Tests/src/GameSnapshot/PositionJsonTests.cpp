#include "ThirdParty/doctest.h"

#include "GameSnapshotJson.h"

using nlohmann::json;

TEST_SUITE("Position::to_json") {

TEST_CASE("a position serializes to an x/y object") {
    json actual = Position{ 3, 5 };

    CHECK(actual == json{ { "x", 3 }, { "y", 5 } });
}

} // TEST_SUITE
