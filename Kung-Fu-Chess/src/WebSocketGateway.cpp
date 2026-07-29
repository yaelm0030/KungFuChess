#include "WebSocketGateway.h"

#include <cassert>
#include <exception>
#include <vector>

#include <nlohmann/json.hpp>

#include "BusChannels.h"
#include "CommandEnvelope.h"
#include "Parser.h"

WebSocketGateway::WebSocketGateway(MessageBus& bus, UserRepository& user_repository)
    : bus_(bus), user_repository_(user_repository) {
    bus_.subscribe(kSnapshotChannel,
        subscription_guard_.wrap([this](const std::string& payload) { broadcast(payload); }));
}

WebSocketGateway::~WebSocketGateway() {
    subscription_guard_.close();
    stop();
}

void WebSocketGateway::start(uint16_t port) {
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
        if (tokens.size() >= 2 && tokens[0] == "name") {
            std::string name = tokens[1];
            for (size_t i = 2; i < tokens.size(); ++i) {
                name += ' ' + tokens[i];
            }

            std::lock_guard<std::mutex> lock(mutex_);
            player_names_[hdl] = name;
            try {
                player_ratings_[hdl] = user_repository_.ensure_user(name);
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
            bus_.publish(kCommandsChannel, nlohmann::json(CommandEnvelope{ *acting_color, msg->get_payload() }).dump());
        }
    });

    ws_server_.listen(port);
    ws_server_.start_accept();
    started_ = true;

    websocketpp::lib::asio::error_code ec;
    auto endpoint = ws_server_.get_local_endpoint(ec);
    // Should never fail right after listen(); assert instead of silently returning 0.
    assert(!ec && "WebSocketGateway::start: get_local_endpoint failed after listen()");
    port_ = ec ? port : endpoint.port();

    io_thread_ = std::thread([this] { ws_server_.run(); });
}

uint16_t WebSocketGateway::port() const {
    return port_;
}

std::optional<std::string> WebSocketGateway::username(Color color) const {
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

std::optional<int> WebSocketGateway::rating(Color color) const {
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

void WebSocketGateway::stop() {
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

    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

void WebSocketGateway::broadcast(const std::string& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const websocketpp::connection_hdl& hdl : connections_) {
        websocketpp::lib::error_code ec;
        ws_server_.send(hdl, payload, websocketpp::frame::opcode::text, ec);
    }
}
