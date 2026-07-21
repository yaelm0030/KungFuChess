#include "GameServer.h"

#include <cassert>
#include <chrono>
#include <exception>

#include <nlohmann/json.hpp>

#include "ClientCommand.h"
#include "GameSnapshotJson.h"

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

void GameServer::start(uint16_t port) {
    if (running_.exchange(true)) {
        return;
    }

    ws_server_.init_asio();
    ws_server_.set_access_channels(websocketpp::log::alevel::none);
    ws_server_.clear_access_channels(websocketpp::log::alevel::all);
    ws_server_.set_error_channels(websocketpp::log::elevel::none);

    ws_server_.set_open_handler([this](websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.insert(hdl);
    });
    ws_server_.set_close_handler([this](websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.erase(hdl);
    });
    ws_server_.set_message_handler([this](websocketpp::connection_hdl, WsServer::message_ptr msg) {
        apply_command(msg->get_payload());
    });

    ws_server_.listen(port);
    ws_server_.start_accept();
    started_ = true;

    websocketpp::lib::asio::error_code ec;
    auto endpoint = ws_server_.get_local_endpoint(ec);
    // Should never fail right after a successful listen(); an assert is more
    // honest here than silently handing back the OS-assign sentinel (0) as
    // if it were the real bound port.
    assert(!ec && "GameServer::start: get_local_endpoint failed after listen()");
    port_ = ec ? port : endpoint.port();

    io_thread_ = std::thread([this] { ws_server_.run(); });
    tick_thread_ = std::thread(&GameServer::run_tick_loop, this);
}

uint16_t GameServer::port() const {
    return port_;
}

void GameServer::stop() {
    running_ = false;

    if (started_) {
        // stop_listening()/stop() mutate transport state (the acceptor, the
        // io_service itself) that io_thread_ concurrently touches via its
        // perpetually-outstanding async_accept; calling them directly from
        // this (foreign) thread would race that. Posting onto ws_server_'s
        // own io_service runs them serialized with everything else already
        // scheduled there instead. ws_server_.stop() makes run() return, so
        // io_thread_ exits on its own once this handler completes.
        //
        // This is an abrupt teardown, not a graceful WS close handshake:
        // stop() halts the io_service before any close frame we might queue
        // here could actually be written, so connected clients just see the
        // TCP connection drop. Nothing in this codebase distinguishes that
        // from a clean close yet, so there's no handshake-draining logic.
        ws_server_.get_io_service().post([this]() {
            websocketpp::lib::error_code ec;
            ws_server_.stop_listening(ec);
            ws_server_.stop();
        });
        started_ = false;
    }

    if (tick_thread_.joinable()) {
        tick_thread_.join();
    }
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

void GameServer::run_tick_loop() {
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

        nlohmann::json json = snapshot();
        broadcast(json.dump());

        next_deadline += std::chrono::milliseconds(kTickMs);
        std::this_thread::sleep_until(next_deadline);
    }
}

void GameServer::broadcast(const std::string& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const websocketpp::connection_hdl& hdl : connections_) {
        websocketpp::lib::error_code ec;
        ws_server_.send(hdl, payload, websocketpp::frame::opcode::text, ec);
    }
}
