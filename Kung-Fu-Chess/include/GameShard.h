#pragma once

#include <deque>
#include <mutex>
#include <string>

#include "CommandEnvelope.h"
#include "Controller.h"
#include "GameEngine.h"
#include "MessageBus.h"

// One game's worth of Controller, driven by commands arriving over a MessageBus
// instead of direct calls; publishes a snapshot after every tick.
class GameShard {
public:
    explicit GameShard(Board board, MessageBus& bus, long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell);

    // Drains commands queued since the last call, applies each, advances the controller,
    // then publishes a GameSnapshotJson dump on kSnapshotChannel. Exactly one publish per call.
    void tick(int milliseconds);

private:
    // bus handler: parses payload as CommandEnvelope JSON and pushes it onto queue_; malformed payloads are dropped.
    void enqueue_command(const std::string& payload);

    Controller controller_;
    MessageBus& bus_;
    std::mutex queue_mutex_; // guards queue_ only; controller_ is touched solely by tick()
    std::deque<CommandEnvelope> queue_;
};
