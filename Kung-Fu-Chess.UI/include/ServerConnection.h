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

// WebSocket client side of GameServer: connects once at construction and, on its own
// io thread, parses every incoming JSON broadcast into a GameSnapshot kept as the
// latest one under mutex_. The render loop polls latest_snapshot() instead of driving
// a Controller directly; outbound click/jump commands go through send(). Thread-safe
// the same way GameServer/TestClient are: send() is safe to call cross-thread
// (websocketpp queues it onto the connection's strand), but close()/the destructor
// must post onto the client's own io_service to serialize with the io thread rather
// than racing it.
class ServerConnection {
public:
    // Sends "name <username>" as soon as the handshake completes (not immediately after
    // construction, since the connection isn't open yet at that point and the send
    // would silently be dropped).
    ServerConnection(const std::string& host, uint16_t port, std::string username);
    ~ServerConnection();

    // Sends a plain-text command line (e.g. "click 50 50") to the server.
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
