#include "ThirdParty/doctest.h"

#include "CommandEnvelope.h"

using nlohmann::json;

TEST_SUITE("CommandEnvelope") {

TEST_CASE("a white envelope round-trips through json") {
    CommandEnvelope original{ Color::w, "move a2 a3" };

    CommandEnvelope actual = json(original).get<CommandEnvelope>();

    CHECK(actual == original);
}

TEST_CASE("a black envelope round-trips through json") {
    CommandEnvelope original{ Color::b, "move a7 a6" };

    CommandEnvelope actual = json(original).get<CommandEnvelope>();

    CHECK(actual == original);
}

TEST_CASE("a missing color field throws out_of_range") {
    json input = json{ { "line", "move a2 a3" } };

    CHECK_THROWS_AS(input.get<CommandEnvelope>(), nlohmann::json::out_of_range);
}

TEST_CASE("a missing line field throws out_of_range") {
    json input = json{ { "color", "w" } };

    CHECK_THROWS_AS(input.get<CommandEnvelope>(), nlohmann::json::out_of_range);
}

} // TEST_SUITE
