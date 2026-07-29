#include "GameShard.h"

#include <chrono>

#include <nlohmann/json.hpp>

#include "BusChannels.h"
#include "ClientCommand.h"
#include "GameSnapshotJson.h"

GameShard::GameShard(Board board, MessageBus& bus, long long move_ms_per_cell)
    : controller_(std::move(board), move_ms_per_cell), bus_(bus) {
    bus_.subscribe(kCommandsChannel,
        subscription_guard_.wrap([this](const std::string& payload) { enqueue_command(payload); }));
}

GameShard::~GameShard() {
    subscription_guard_.close();
    stop();
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

void GameShard::start() {
    if (running_.exchange(true)) {
        return;
    }
    tick_thread_ = std::thread(&GameShard::run_tick_loop, this);
}

void GameShard::stop() {
    running_ = false;
    if (tick_thread_.joinable()) {
        tick_thread_.join();
    }
}

void GameShard::run_tick_loop() {
    // Absolute deadline avoids drift from repeated sleep_for calls.
    auto next_deadline = std::chrono::steady_clock::now();
    while (running_) {
        try {
            tick(kTickMs);
        } catch (const std::exception&) {
            // Stop instead of retrying into the same failure every tick.
            running_ = false;
            break;
        }

        next_deadline += std::chrono::milliseconds(kTickMs);
        std::this_thread::sleep_until(next_deadline);
    }
}
