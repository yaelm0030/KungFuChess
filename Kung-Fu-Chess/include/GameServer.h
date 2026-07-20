#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <set>
#include <string>
#include <thread>

// websocketpp's handshake helpers (md5/sha1/frame) trip MSVC's C4267 under
// our warning level; not our code to fix, so silence it just for the vendor
// headers (MSVC ties template warning state to the definition site, so this
// covers instantiations pulled in from GameServer.cpp too).
#pragma warning(push, 0)
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#pragma warning(pop)

#include "Controller.h"
#include "GameEngine.h"
#include "GameSnapshot.h"

// Sole gateway to Controller, now with a real WebSocket transport: accepts
// connections and broadcasts a JSON snapshot after every tick. No inbound
// message handling yet (clients receive pushes; sending a command from a
// client is a later step). Thread-safe: tick/apply_command/snapshot and the
// connection open/close handlers all lock mutex_, so the tick thread, the
// io thread, and a caller thread can safely share one GameServer.
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

    // Starts listening on port (0 = let the OS pick a free port) and spawns
    // the tick thread and the io thread. No-op if already running. Calling
    // start() again after stop() is untested and not currently supported.
    void start(uint16_t port);

    // The actual bound port; only meaningful after start() has returned.
    uint16_t port() const;

    // Stops accepting connections, closes the websocket server, and joins
    // both the tick thread and the io thread. Idempotent; safe if never
    // started. Not safe to call concurrently with itself or the destructor
    // from another thread (would double-join); GameServer has one owner.
    void stop();

private:
    void run_tick_loop();
    void broadcast(const std::string& payload);

    using WsServer = websocketpp::server<websocketpp::config::asio>;

    Controller controller_;
    mutable std::mutex mutex_;
    std::thread tick_thread_;
    std::thread io_thread_;
    std::atomic<bool> running_{ false };

    WsServer ws_server_;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_;
    uint16_t port_{ 0 };
    bool started_{ false };
};
