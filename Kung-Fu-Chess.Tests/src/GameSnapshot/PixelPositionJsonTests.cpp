#include "ThirdParty/doctest.h"

#include "GameSnapshotJson.h"

using nlohmann::json;

TEST_SUITE("PixelPosition::to_json") {

TEST_CASE("a pixel position serializes to an x/y object") {
    json actual = PixelPosition{ 120, 40 };

    CHECK(actual == json{ { "x", 120 }, { "y", 40 } });
}

} // TEST_SUITE
