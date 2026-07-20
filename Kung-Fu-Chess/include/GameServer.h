#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include "Controller.h"
#include "GameEngine.h"
#include "GameSnapshot.h"

// Sole gateway to Controller. Thread-safe: tick/apply_command/snapshot all lock
// mutex_, so the background tick thread and a caller thread can safely share
// one GameServer. Still no socket/networking of its own.
class GameServer {
public:
    static constexpr int kTickMs = 16;

    explicit GameServer(Board board, long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell);
    ~GameServer();

    // Advances the game clock; the only way time moves forward.
    void tick(int milliseconds);

    // Applies a click/jump line via ClientCommand; malformed or non-click/jump
    // lines (e.g. "wait") are silently ignored, same policy as ClientCommand.
    void apply_command(const std::string& line);

    GameSnapshot snapshot() const;

    // Spawns a background thread ticking every kTickMs. No-op if already running.
    void start();

    // Stops the background thread and joins it. Idempotent; safe if never started.
    // Not safe to call concurrently with itself or the destructor from another
    // thread (would double-join the same std::thread); GameServer has one owner.
    void stop();

private:
    void run();

    Controller controller_;
    mutable std::mutex mutex_;
    std::thread thread_;
    std::atomic<bool> running_{ false };
};
