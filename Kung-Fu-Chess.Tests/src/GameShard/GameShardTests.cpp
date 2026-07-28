#include "ThirdParty/doctest.h"

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "BusChannels.h"
#include "Constants.h"
#include "GameEngine.h"
#include "GameShard.h"
#include "GameSnapshotJson.h"
#include "Parser.h"
#include "Position.h"
#include "../MessageBus/InProcessMessageBus.h"

namespace {

Board make_board() {
    return Parser::parse_board({
        "bR bN .",
        ".  .  .",
        "wR .  wN",
    });
}

std::optional<PieceSnapshot> find_piece(const GameSnapshot& snap, Color color, PieceType type) {
    for (const PieceSnapshot& piece : snap.pieces) {
        if (piece.color == color && piece.type == type) {
            return piece;
        }
    }
    return std::nullopt;
}

std::string select_white_rook_command() {
    return nlohmann::json{ { "color", "w" }, { "line", "click 50 250" } }.dump(); // wR at (0,2)
}

} // namespace

TEST_SUITE("GameShard::tick") {

TEST_CASE("a command queued before tick() is applied that tick and reflected in the published snapshot") {
    InProcessMessageBus bus;
    std::string captured;
    bus.subscribe(kSnapshotChannel, [&](const std::string& payload) { captured = payload; });

    GameShard shard(make_board(), bus);
    bus.publish(kCommandsChannel, select_white_rook_command());
    shard.tick(16);

    GameSnapshot snap = nlohmann::json::parse(captured).get<GameSnapshot>();
    CHECK(snap.selected_w == Position{ 0, 2 });
    CHECK(snap.selected == std::nullopt);
}

TEST_CASE("multiple commands queued before a single tick are all applied, in order, within that tick") {
    InProcessMessageBus bus;
    std::string captured;
    bus.subscribe(kSnapshotChannel, [&](const std::string& payload) { captured = payload; });

    GameShard shard(make_board(), bus);
    bus.publish(kCommandsChannel, select_white_rook_command());
    bus.publish(kCommandsChannel, nlohmann::json{ { "color", "w" }, { "line", "click 50 150" } }.dump()); // -> (0,1), one cell up

    shard.tick(static_cast<int>(GameEngine::kDefaultMoveMsPerCell));

    GameSnapshot snap = nlohmann::json::parse(captured).get<GameSnapshot>();
    std::optional<PieceSnapshot> wr = find_piece(snap, Color::w, PieceType::R);
    REQUIRE(wr.has_value());
    CHECK(wr->pixels_location.x == 0);
    CHECK(wr->pixels_location.y == constants::kCellSizePx);
}

TEST_CASE("tick() publishes exactly one snapshot message, whether or not commands were queued") {
    InProcessMessageBus bus;
    int message_count = 0;
    bus.subscribe(kSnapshotChannel, [&](const std::string&) { ++message_count; });

    GameShard shard(make_board(), bus);

    SUBCASE("no commands queued") {
    }
    SUBCASE("one command queued") {
        bus.publish(kCommandsChannel, select_white_rook_command());
    }

    shard.tick(16);

    CHECK(message_count == 1);
}

TEST_CASE("a malformed payload on the commands channel is dropped without crashing, and later valid commands still apply") {
    InProcessMessageBus bus;
    std::string captured;
    bus.subscribe(kSnapshotChannel, [&](const std::string& payload) { captured = payload; });

    GameShard shard(make_board(), bus);
    bus.publish(kCommandsChannel, "not valid json");
    shard.tick(16);

    GameSnapshot snap = nlohmann::json::parse(captured).get<GameSnapshot>();
    CHECK(snap.selected == std::nullopt);
    CHECK(snap.selected_w == std::nullopt);
    CHECK(snap.selected_b == std::nullopt);

    bus.publish(kCommandsChannel, select_white_rook_command());
    shard.tick(16);

    snap = nlohmann::json::parse(captured).get<GameSnapshot>();
    CHECK(snap.selected_w == Position{ 0, 2 });
}

TEST_CASE("a well-formed payload with an invalid color is dropped without crashing, and later valid commands still apply") {
    InProcessMessageBus bus;
    std::string captured;
    bus.subscribe(kSnapshotChannel, [&](const std::string& payload) { captured = payload; });

    GameShard shard(make_board(), bus);
    bus.publish(kCommandsChannel, nlohmann::json{ { "color", "z" }, { "line", "click 50 250" } }.dump());
    shard.tick(16);

    GameSnapshot snap = nlohmann::json::parse(captured).get<GameSnapshot>();
    CHECK(snap.selected == std::nullopt);
    CHECK(snap.selected_w == std::nullopt);
    CHECK(snap.selected_b == std::nullopt);

    bus.publish(kCommandsChannel, select_white_rook_command());
    shard.tick(16);

    snap = nlohmann::json::parse(captured).get<GameSnapshot>();
    CHECK(snap.selected_w == Position{ 0, 2 });
}

}
