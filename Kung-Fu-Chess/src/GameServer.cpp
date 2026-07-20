#include "GameServer.h"

#include <chrono>
#include <exception>

#include "ClientCommand.h"

GameServer::GameServer(Board board, long long move_ms_per_cell) : controller_(std::move(board), move_ms_per_cell) {
}

GameServer::~GameServer() {
    stop();
}

void GameServer::tick(int milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    controller_.wait(milliseconds);
}

void GameServer::apply_command(const std::string& line) {
    std::lock_guard<std::mutex> lock(mutex_);
    ClientCommand::apply(controller_, line);
}

GameSnapshot GameServer::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return controller_.snapshot();
}

void GameServer::start() {
    if (running_.exchange(true)) {
        return;
    }
    thread_ = std::thread(&GameServer::run, this);
}

void GameServer::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void GameServer::run() {
    // sleep_until against an absolute deadline (rather than repeated sleep_for(kTickMs))
    // avoids compounding drift from the OS timer's coarse resolution, keeping the
    // virtual clock from lagging behind wall-clock time over many iterations.
    auto next_deadline = std::chrono::steady_clock::now();
    while (running_) {
        try {
            tick(kTickMs);
        } catch (const std::exception&) {
            // Stop rather than retry into the same failure every kTickMs;
            // pre-thread, an exception from tick() would have reached the caller.
            running_ = false;
            break;
        }
        next_deadline += std::chrono::milliseconds(kTickMs);
        std::this_thread::sleep_until(next_deadline);
    }
}
