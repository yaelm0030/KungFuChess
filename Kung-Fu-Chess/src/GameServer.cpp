#include "GameServer.h"

#include <cassert>
#include <chrono>
#include <exception>
#include <vector>

#include <nlohmann/json.hpp>

#include "ClientCommand.h"
#include "GameSnapshotJson.h"
#include "Parser.h"

GameServer::GameServer(Board board, UserRepository& user_repository, long long move_ms_per_cell)
    : controller_(std::move(board), move_ms_per_cell), user_repository_(user_repository) {
}

GameServer::~GameServer() {
    stop();
}

void GameServer::tick(int milliseconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    controller_.wait(milliseconds);
}

void GameServer::apply_command(const std::string& line, std::optional<Color> acting_color) {
    std::lock_guard<std::mutex> lock(mutex_);
    ClientCommand::apply(controller_, line, acting_color);
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

        // Occupancy-based, so a freed color slot is reused; extra connections are spectators.
        bool white_taken = false;
        bool black_taken = false;
        for (const auto& entry : player_colors_) {
            if (entry.second == Color::w) {
                white_taken = true;
            } else {
                black_taken = true;
            }
        }
        if (!white_taken) {
            player_colors_[hdl] = Color::w;
        } else if (!black_taken) {
            player_colors_[hdl] = Color::b;
        }
    });
    ws_server_.set_close_handler([this](websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.erase(hdl);
        player_colors_.erase(hdl);
        player_names_.erase(hdl);
        player_ratings_.erase(hdl);
    });
    ws_server_.set_message_handler([this](websocketpp::connection_hdl hdl, WsServer::message_ptr msg) {
        std::vector<std::string> tokens = Parser::tokenize(msg->get_payload());
        if (tokens.size() == 2 && tokens[0] == "name") {
            std::lock_guard<std::mutex> lock(mutex_);
            player_names_[hdl] = tokens[1];
            try {
                player_ratings_[hdl] = user_repository_.ensure_user(tokens[1]);
            } catch (const std::exception&) {
                // Swallow: a DB hiccup for one player must not affect everyone else.
            }
            return;
        }

        std::optional<Color> acting_color;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = player_colors_.find(hdl);
            if (it != player_colors_.end()) {
                acting_color = it->second;
            }
        }
        if (acting_color.has_value()) {
            apply_command(msg->get_payload(), acting_color);
        }
    });

    ws_server_.listen(port);
    ws_server_.start_accept();
    started_ = true;

    websocketpp::lib::asio::error_code ec;
    auto endpoint = ws_server_.get_local_endpoint(ec);
    // Should never fail right after listen(); assert instead of silently returning 0.
    assert(!ec && "GameServer::start: get_local_endpoint failed after listen()");
    port_ = ec ? port : endpoint.port();

    io_thread_ = std::thread([this] { ws_server_.run(); });
    tick_thread_ = std::thread(&GameServer::run_tick_loop, this);
}

uint16_t GameServer::port() const {
    return port_;
}

std::optional<std::string> GameServer::username(Color color) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : player_colors_) {
        if (entry.second == color) {
            auto it = player_names_.find(entry.first);
            if (it != player_names_.end()) {
                return it->second;
            }
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<int> GameServer::rating(Color color) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : player_colors_) {
        if (entry.second == color) {
            auto it = player_ratings_.find(entry.first);
            if (it != player_ratings_.end()) {
                return it->second;
            }
            return std::nullopt;
        }
    }
    return std::nullopt;
}

void GameServer::stop() {
    running_ = false;

    if (started_) {
        // Must run on ws_server_'s own io thread to avoid racing it.
        // This is an abrupt teardown; clients just see the TCP connection drop.
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
