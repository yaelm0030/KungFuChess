#include "ThirdParty/doctest.h"

#include "GameSnapshotJson.h"

using nlohmann::json;

TEST_SUITE("PieceSnapshot::to_json") {

TEST_CASE("a piece snapshot serializes all its fields") {
    PieceSnapshot piece{ PieceType::Q, Color::b, { 10, 20 }, { 30, 40 }, 0.5, PieceState::move, 0.75 };

    json actual = piece;

    CHECK(actual == json{
        { "type", "Q" },
        { "color", "b" },
        { "pixels_location", { { "x", 10 }, { "y", 20 } } },
        { "target_pixels_location", { { "x", 30 }, { "y", 40 } } },
        { "progress", 0.5 },
        { "state", "move" },
        { "cooldown_progress", 0.75 },
    });
}

} // TEST_SUITE
