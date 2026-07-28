#include "ThirdParty/doctest.h"

#include <chrono>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "BusChannels.h"
#include "Constants.h"
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

TEST_SUITE("GameShard::start/stop") {

TEST_CASE("start then stop returns promptly without hanging or crashing, having published at least one snapshot") {
    InProcessMessageBus bus;
    int snapshot_count = 0;
    bus.subscribe(kSnapshotChannel, [&](const std::string&) { ++snapshot_count; });

    GameShard shard(make_board(), bus);

    shard.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    shard.stop();

    CHECK(snapshot_count >= 1);
}

TEST_CASE("the tick thread applies a queued command and advances the clock on its own, with no external tick() call") {
    const long long move_ms_per_cell = 100; // small, so the test proves this in tens of ms, not over a second
    InProcessMessageBus bus;
    std::string captured;
    bus.subscribe(kSnapshotChannel, [&](const std::string& payload) { captured = payload; });

    GameShard shard(make_board(), bus, move_ms_per_cell);
    bus.publish(kCommandsChannel, select_white_rook_command());
    bus.publish(kCommandsChannel, nlohmann::json{ { "color", "w" }, { "line", "click 50 150" } }.dump()); // -> (0,1), one cell up

    shard.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(move_ms_per_cell + 100));
    shard.stop();

    GameSnapshot snap = nlohmann::json::parse(captured).get<GameSnapshot>();
    std::optional<PieceSnapshot> wr = find_piece(snap, Color::w, PieceType::R);
    REQUIRE(wr.has_value());
    CHECK(wr->pixels_location.x == 0);
    CHECK(wr->pixels_location.y == constants::kCellSizePx);
}

TEST_CASE("stop is idempotent when called twice in a row") {
    InProcessMessageBus bus;
    int snapshot_count = 0;
    bus.subscribe(kSnapshotChannel, [&](const std::string&) { ++snapshot_count; });

    GameShard shard(make_board(), bus);

    shard.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    shard.stop();
    shard.stop();

    CHECK(snapshot_count >= 1);
}

TEST_CASE("a shard that goes out of scope while still running tears down cleanly") {
    {
        InProcessMessageBus bus;
        GameShard shard(make_board(), bus);
        shard.start();
    }

    CHECK(true);
}

}
