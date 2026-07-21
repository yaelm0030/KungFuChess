#include "ServerConnection.h"

#include <stdexcept>

#include <nlohmann/json.hpp>

#include "GameSnapshotJson.h"

ServerConnection::ServerConnection(const std::string& host, uint16_t port) {
    client_.init_asio();
    client_.set_access_channels(websocketpp::log::alevel::none);
    client_.clear_access_channels(websocketpp::log::alevel::all);
    client_.set_error_channels(websocketpp::log::elevel::none);

    client_.set_message_handler([this](websocketpp::connection_hdl, WsClient::message_ptr msg) {
        // The server is the only sender and always emits a valid GameSnapshot, but a
        // corrupt/partial message shouldn't take the whole client down: skip it and
        // keep showing the last good snapshot, same tolerance ClientCommand applies
        // to malformed input on the server side.
        try {
            GameSnapshot snapshot = nlohmann::json::parse(msg->get_payload()).get<GameSnapshot>();
            std::lock_guard<std::mutex> lock(mutex_);
            latest_snapshot_ = std::move(snapshot);
        } catch (const std::exception&) {
        }
    });

    std::string uri = "ws://" + host + ":" + std::to_string(port);
    websocketpp::lib::error_code ec;
    WsClient::connection_ptr connection = client_.get_connection(uri, ec);
    if (ec) {
        throw std::runtime_error("ServerConnection: failed to create connection to " + uri + ": " + ec.message());
    }

    hdl_ = connection->get_handle();
    client_.connect(connection);
    io_thread_ = std::thread([this] { client_.run(); });
}

ServerConnection::~ServerConnection() {
    close();
}

void ServerConnection::send(const std::string& line) {
    websocketpp::lib::error_code ec;
    client_.send(hdl_, line, websocketpp::frame::opcode::text, ec);
}

std::optional<GameSnapshot> ServerConnection::latest_snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return latest_snapshot_;
}

void ServerConnection::close() {
    if (closed_.exchange(true)) {
        return;
    }

    // Same race avoided as GameServer::stop(): close()/stop() mutate io_service state
    // that io_thread_ concurrently touches, so they must run on that thread via post(),
    // not be called directly from here.
    client_.get_io_service().post([this]() {
        websocketpp::lib::error_code ec;
        client_.close(hdl_, websocketpp::close::status::normal, "", ec);
        client_.stop();
    });

    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}
