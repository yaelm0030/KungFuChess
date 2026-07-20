#include "ThirdParty/doctest.h"

#include "GameSnapshotJson.h"

using nlohmann::json;

TEST_SUITE("enum to_json") {

TEST_CASE("Color serializes to its single-letter token") {
    CHECK(json(Color::w) == json("w"));
    CHECK(json(Color::b) == json("b"));
}

TEST_CASE("PieceType serializes to its letter token, distinguishing K from N") {
    CHECK(json(PieceType::K) == json("K"));
    CHECK(json(PieceType::N) == json("N"));
}

TEST_CASE("PieceState serializes to its name token") {
    CHECK(json(PieceState::jump) == json("jump"));
    CHECK(json(PieceState::short_rest) == json("short_rest"));
}

} // TEST_SUITE
