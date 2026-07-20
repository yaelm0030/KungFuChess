#include "GameServer.h"

#include "ClientCommand.h"

GameServer::GameServer(Board board, long long move_ms_per_cell) : controller_(std::move(board), move_ms_per_cell) {
}

void GameServer::tick(int milliseconds) {
    controller_.wait(milliseconds);
}

void GameServer::apply_command(const std::string& line) {
    ClientCommand::apply(controller_, line);
}

GameSnapshot GameServer::snapshot() const {
    return controller_.snapshot();
}
