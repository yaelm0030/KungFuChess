#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

#include "CommandEnvelope.h"
#include "Controller.h"
#include "GameEngine.h"
#include "MessageBus.h"

// One game's worth of Controller, driven by commands arriving over a MessageBus
// instead of direct calls; publishes a snapshot after every tick.
class GameShard {
public:
    static constexpr int kTickMs = 16;

    explicit GameShard(Board board, MessageBus& bus, long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell);
    ~GameShard();

    // Drains commands queued since the last call, applies each, advances the controller,
    // then publishes a snapshot. Exactly one publish per call. Not for concurrent use with
    // a running tick thread — use either manual tick() or start()/stop(), not both.
    void tick(int milliseconds);

    // Spawns a thread that calls tick(kTickMs) on a fixed interval until stop(). No-op if already running.
    void start();

    // Stops the tick thread and joins it. Idempotent, but not safe to call concurrently with itself.
    void stop();

private:
    // bus handler: parses payload as CommandEnvelope JSON and pushes it onto queue_; malformed payloads are dropped.
    void enqueue_command(const std::string& payload);
    void run_tick_loop();

    Controller controller_;
    MessageBus& bus_;
    std::mutex queue_mutex_; // guards queue_ only; controller_ is touched solely by tick()
    std::deque<CommandEnvelope> queue_;

    std::thread tick_thread_;
    std::atomic<bool> running_{ false };
};
