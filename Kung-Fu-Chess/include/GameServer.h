#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
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
#include "Types.h"
#include "UserRepository.h"

// Sole gateway to Controller, now with a real WebSocket transport: accepts
// connections, broadcasts a JSON snapshot after every tick, and applies
// click/jump commands sent by clients. Thread-safe: tick/apply_command/
// snapshot and the connection open/close/message handlers all lock mutex_,
// so the tick thread, the io thread, and a caller thread can safely share
// one GameServer.
class GameServer {
public:
    static constexpr int kTickMs = 16;

    explicit GameServer(Board board, UserRepository& user_repository,
                         long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell);
    ~GameServer();

    // Advances the game clock; the only way time moves forward.
    void tick(int milliseconds);

    // Applies a click/jump line via ClientCommand; malformed or non-click/jump
    // lines (e.g. "wait") are silently ignored, same policy as ClientCommand.
    // acting_color is forwarded to ClientCommand::apply; see Controller::click for its meaning.
    void apply_command(const std::string& line, std::optional<Color> acting_color = std::nullopt);

    GameSnapshot snapshot() const;

    // Starts listening on port (0 = let the OS pick a free port) and spawns
    // the tick thread and the io thread. No-op if already running. Calling
    // start() again after stop() is untested and not currently supported.
    void start(uint16_t port);

    // The actual bound port; only meaningful after start() has returned.
    uint16_t port() const;

    // The username last sent by color's connection via a "name <username>"
    // message, or nullopt if it never sent one (or has no connection at all).
    std::optional<std::string> username(Color color) const;

    // The rating resolved from user_repository_ for color's last "name"
    // message, or nullopt if it never sent one (no connection, or the
    // repository lookup failed and was swallowed).
    std::optional<int> rating(Color color) const;

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
    UserRepository& user_repository_;
    mutable std::mutex mutex_;
    std::thread tick_thread_;
    std::thread io_thread_;
    std::atomic<bool> running_{ false };

    WsServer ws_server_;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections_;
    std::map<websocketpp::connection_hdl, Color, std::owner_less<websocketpp::connection_hdl>> player_colors_;
    std::map<websocketpp::connection_hdl, std::string, std::owner_less<websocketpp::connection_hdl>> player_names_;
    std::map<websocketpp::connection_hdl, int, std::owner_less<websocketpp::connection_hdl>> player_ratings_;
    uint16_t port_{ 0 };
    bool started_{ false };
};
