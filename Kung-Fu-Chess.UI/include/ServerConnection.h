#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

// See GameServer.h (engine side) for why the vendor include is wrapped like this.
#pragma warning(push, 0)
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#pragma warning(pop)

#include "GameSnapshot.h"

// WebSocket client for GameServer; stores the latest broadcast snapshot under mutex_.
// close() must post onto the io thread to avoid racing it.
class ServerConnection {
public:
    // Sends "name <username>" on handshake completion; earlier, the send would be dropped.
    ServerConnection(const std::string& host, uint16_t port, std::string username);
    ~ServerConnection();

    void send(const std::string& line);

    // The most recently received snapshot, or nullopt before the first one arrives.
    std::optional<GameSnapshot> latest_snapshot() const;

    // Closes the connection and joins the io thread. Idempotent.
    void close();

private:
    using WsClient = websocketpp::client<websocketpp::config::asio_client>;

    WsClient client_;
    websocketpp::connection_hdl hdl_;
    std::thread io_thread_;
    std::atomic<bool> closed_{ false };

    mutable std::mutex mutex_;
    std::optional<GameSnapshot> latest_snapshot_;
};
