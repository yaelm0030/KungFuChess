#include "GameShard.h"

#include <nlohmann/json.hpp>

#include "BusChannels.h"
#include "ClientCommand.h"
#include "GameSnapshotJson.h"

GameShard::GameShard(Board board, MessageBus& bus, long long move_ms_per_cell)
    : controller_(std::move(board), move_ms_per_cell), bus_(bus) {
    bus_.subscribe(kCommandsChannel, [this](const std::string& payload) { enqueue_command(payload); });
}

void GameShard::tick(int milliseconds) {
    std::deque<CommandEnvelope> commands;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        commands.swap(queue_);
    }

    for (const CommandEnvelope& envelope : commands) {
        ClientCommand::apply(controller_, envelope.line, envelope.color);
    }
    controller_.wait(milliseconds);

    nlohmann::json snapshot_json = controller_.snapshot();
    bus_.publish(kSnapshotChannel, snapshot_json.dump());
}

void GameShard::enqueue_command(const std::string& payload) {
    try {
        CommandEnvelope envelope = nlohmann::json::parse(payload).get<CommandEnvelope>();
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.push_back(std::move(envelope));
    } catch (const nlohmann::json::exception&) {
        // Malformed payload (bad JSON or missing field); dropped like ClientCommand's unknown-input convention.
    }
}
