#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>

// Silences an MSVC C4267 warning from websocketpp's vendor headers.
#pragma warning(push, 0)
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#pragma warning(pop)

#include "Controller.h"
#include "GameEngine.h"
#include "GameSnapshot.h"
#include "Types.h"
#include "UserRepository.h"

// WebSocket front end for Controller; mutex_ makes it thread-safe across the tick and io threads.
class GameServer {
public:
    static constexpr int kTickMs = 16;

    explicit GameServer(Board board, UserRepository& user_repository,
                         long long move_ms_per_cell = GameEngine::kDefaultMoveMsPerCell);
    ~GameServer();

    // Advances the game clock; the only way time moves forward.
    void tick(int milliseconds);

    // Applies a click/jump line via ClientCommand; unknown lines are ignored.
    void apply_command(const std::string& line, std::optional<Color> acting_color = std::nullopt);

    GameSnapshot snapshot() const;

    // Starts listening (port 0 = OS-assigned) and spawns the tick/io threads. No-op if already running.
    void start(uint16_t port);

    // The actual bound port; only meaningful after start() has returned.
    uint16_t port() const;

    // The last "name" message from color's connection, or nullopt if none.
    std::optional<std::string> username(Color color) const;

    // Rating from the last "name" message, or nullopt if none or the DB lookup failed.
    std::optional<int> rating(Color color) const;

    // Stops the server and joins both threads. Idempotent, but not safe to call concurrently with itself.
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
