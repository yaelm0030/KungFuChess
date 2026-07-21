#include "ThirdParty/doctest.h"

#include "GameSnapshotJson.h"

using nlohmann::json;

TEST_SUITE("PixelPosition::to_json") {

TEST_CASE("a pixel position serializes to an x/y object") {
    json actual = PixelPosition{ 120, 40 };

    CHECK(actual == json{ { "x", 120 }, { "y", 40 } });
}

} // TEST_SUITE

TEST_SUITE("PixelPosition::from_json") {

TEST_CASE("an x/y object round-trips through a pixel position") {
    json input = json{ { "x", 120 }, { "y", 40 } };

    PixelPosition parsed = input.get<PixelPosition>();

    CHECK(json(parsed) == input);
}

} // TEST_SUITE
