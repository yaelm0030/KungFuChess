#pragma once

#include <string>

#include "Controller.h"
#include "GameEngine.h"
#include "GameSnapshot.h"

// Sole gateway to Controller: single-threaded, no locking, no I/O of its own.
// Later steps layer a socket/thread on top without touching this contract.
class GameServer {
public:
    explicit GameServer(Board board, long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell);

    // Advances the game clock; the only way time moves forward.
    void tick(int milliseconds);

    // Applies a click/jump line via ClientCommand; malformed or non-click/jump
    // lines (e.g. "wait") are silently ignored, same policy as ClientCommand.
    void apply_command(const std::string& line);

    GameSnapshot snapshot() const;

private:
    Controller controller_;
};
