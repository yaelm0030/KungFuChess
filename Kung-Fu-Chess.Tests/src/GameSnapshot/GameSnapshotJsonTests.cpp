#include "ThirdParty/doctest.h"

#include "GameSnapshotJson.h"

using nlohmann::json;

TEST_SUITE("GameSnapshot::to_json") {

TEST_CASE("an empty snapshot serializes with empty arrays and a null selected") {
    GameSnapshot snapshot{ 0, 0, {}, false };

    json actual = snapshot;

    CHECK(actual == json{
        { "board_width", 0 },
        { "board_height", 0 },
        { "pieces", json::array() },
        { "is_game_over", false },
        { "score_w", 0 },
        { "score_b", 0 },
        { "move_history", json::array() },
        { "selected", nullptr },
    });
}

TEST_CASE("a non-trivial snapshot serializes pieces, move_history, and selected") {
    GameSnapshot snapshot;
    snapshot.board_width = 3;
    snapshot.board_height = 3;
    snapshot.pieces = {
        PieceSnapshot{ PieceType::K, Color::w, { 0, 0 }, { 0, 0 }, 1.0, PieceState::idle, 0.0 },
        PieceSnapshot{ PieceType::P, Color::b, { 60, 0 }, { 60, 60 }, 0.5, PieceState::move, 0.0 },
    };
    snapshot.is_game_over = false;
    snapshot.score_w = 3;
    snapshot.score_b = 0;
    snapshot.move_history = { MoveRecord{ PieceType::P, Color::b, { 1, 0 }, { 1, 2 } } };
    snapshot.selected = Position{ 1, 2 };

    json actual = snapshot;

    CHECK(actual == json{
        { "board_width", 3 },
        { "board_height", 3 },
        { "pieces", json::array({
            json{
                { "type", "K" },
                { "color", "w" },
                { "pixels_location", { { "x", 0 }, { "y", 0 } } },
                { "target_pixels_location", { { "x", 0 }, { "y", 0 } } },
                { "progress", 1.0 },
                { "state", "idle" },
                { "cooldown_progress", 0.0 },
            },
            json{
                { "type", "P" },
                { "color", "b" },
                { "pixels_location", { { "x", 60 }, { "y", 0 } } },
                { "target_pixels_location", { { "x", 60 }, { "y", 60 } } },
                { "progress", 0.5 },
                { "state", "move" },
                { "cooldown_progress", 0.0 },
            },
        }) },
        { "is_game_over", false },
        { "score_w", 3 },
        { "score_b", 0 },
        { "move_history", json::array({
            json{
                { "type", "P" },
                { "color", "b" },
                { "source", { { "x", 1 }, { "y", 0 } } },
                { "destination", { { "x", 1 }, { "y", 2 } } },
            },
        }) },
        { "selected", { { "x", 1 }, { "y", 2 } } },
    });
}

} // TEST_SUITE

TEST_SUITE("GameSnapshot::from_json") {

TEST_CASE("an empty snapshot round-trips through empty arrays and a null selected") {
    json input = json{
        { "board_width", 0 },
        { "board_height", 0 },
        { "pieces", json::array() },
        { "is_game_over", false },
        { "score_w", 0 },
        { "score_b", 0 },
        { "move_history", json::array() },
        { "selected", nullptr },
    };

    GameSnapshot parsed = input.get<GameSnapshot>();

    CHECK(json(parsed) == input);
}

TEST_CASE("a non-trivial snapshot round-trips pieces, move_history, and selected") {
    json input = json{
        { "board_width", 3 },
        { "board_height", 3 },
        { "pieces", json::array({
            json{
                { "type", "K" },
                { "color", "w" },
                { "pixels_location", { { "x", 0 }, { "y", 0 } } },
                { "target_pixels_location", { { "x", 0 }, { "y", 0 } } },
                { "progress", 1.0 },
                { "state", "idle" },
                { "cooldown_progress", 0.0 },
            },
            json{
                { "type", "P" },
                { "color", "b" },
                { "pixels_location", { { "x", 60 }, { "y", 0 } } },
                { "target_pixels_location", { { "x", 60 }, { "y", 60 } } },
                { "progress", 0.5 },
                { "state", "move" },
                { "cooldown_progress", 0.0 },
            },
        }) },
        { "is_game_over", false },
        { "score_w", 3 },
        { "score_b", 0 },
        { "move_history", json::array({
            json{
                { "type", "P" },
                { "color", "b" },
                { "source", { { "x", 1 }, { "y", 0 } } },
                { "destination", { { "x", 1 }, { "y", 2 } } },
            },
        }) },
        { "selected", { { "x", 1 }, { "y", 2 } } },
    };

    GameSnapshot parsed = input.get<GameSnapshot>();

    CHECK(json(parsed) == input);
}

TEST_CASE("an omitted selected key deserializes to nullopt without throwing") {
    json input = json{
        { "board_width", 0 },
        { "board_height", 0 },
        { "pieces", json::array() },
        { "is_game_over", false },
        { "score_w", 0 },
        { "score_b", 0 },
        { "move_history", json::array() },
    };

    GameSnapshot parsed;
    REQUIRE_NOTHROW(parsed = input.get<GameSnapshot>());

    CHECK(parsed.selected == std::nullopt);
}

} // TEST_SUITE
