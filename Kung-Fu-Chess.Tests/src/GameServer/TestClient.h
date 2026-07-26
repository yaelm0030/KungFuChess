#pragma once

#include "ThirdParty/doctest.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// See GameServer.h for why the vendor include is wrapped like this.
#pragma warning(push, 0)
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#pragma warning(pop)

using WsClient = websocketpp::client<websocketpp::config::asio_client>;

// A minimal test-side websocket client: connects to ws://localhost:<port>,
// collects every text message received on its own io thread, and can send
// commands back to the server. Message waits are timeout-bounded and
// close() forces its io thread to stop promptly, so a broken server fails
// the test instead of hanging it. Shared by every GameServer test file that
// needs a real websocket client, so its close()/send() concurrency fixes
// only ever need applying once.
class TestClient {
public:
    explicit TestClient(uint16_t port) {
        client_.init_asio();
        client_.set_access_channels(websocketpp::log::alevel::none);
        client_.clear_access_channels(websocketpp::log::alevel::all);
        client_.set_error_channels(websocketpp::log::elevel::none);

        client_.set_message_handler([this](websocketpp::connection_hdl, WsClient::message_ptr msg) {
            std::lock_guard<std::mutex> lock(mutex_);
            messages_.push_back(msg->get_payload());
            cv_.notify_all();
        });
        // Lets close() stop the io_service as soon as the close handshake
        // actually completes, instead of always waiting out its grace timer.
        client_.set_close_handler([this](websocketpp::connection_hdl) { client_.stop(); });

        websocketpp::lib::error_code ec;
        WsClient::connection_ptr con = client_.get_connection("ws://localhost:" + std::to_string(port), ec);
        REQUIRE_FALSE(ec);
        hdl_ = con->get_handle();
        client_.connect(con);
        io_thread_ = std::thread([this] { client_.run(); });
    }

    ~TestClient() {
        close();
    }

    // Sends a text message to the server. Safe to call from any thread, same
    // as GameServer::broadcast() calling ws_server_.send() from the tick
    // thread rather than the io thread.
    void send(const std::string& text) {
        websocketpp::lib::error_code ec;
        client_.send(hdl_, text, websocketpp::frame::opcode::text, ec);
    }

    // Waits until at least `count` messages have arrived or the timeout
    // elapses; returns whether that many arrived in time.
    bool wait_for_messages(size_t count, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [this, count] { return messages_.size() >= count; });
    }

    std::string last_message() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_.back();
    }

    // Closes the connection and joins the io thread. Idempotent. Like
    // GameServer::stop(), client_.stop() mutates io_service state that
    // io_thread_ concurrently touches, so it must run on that thread, not be
    // called cross-thread here. client_.close() only queues the close frame;
    // stopping the io_service right after that (as before) could cut it off
    // before it — or a send() issued just before close() — actually reached
    // the wire. So stop() is deferred: the close_handler above fires it as
    // soon as the handshake completes, and set_timer() bounds the wait with
    // a short grace period for when it doesn't (e.g. no server listening),
    // so a broken server still fails the test instead of hanging it.
    void close() {
        if (closed_.exchange(true)) {
            return;
        }
        client_.get_io_service().post([this]() {
            websocketpp::lib::error_code ec;
            client_.close(hdl_, websocketpp::close::status::normal, "", ec);
            client_.set_timer(kCloseGraceMs, [this](websocketpp::lib::error_code) { client_.stop(); });
        });
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
    }

private:
    static constexpr long kCloseGraceMs = 100;

    WsClient client_;
    websocketpp::connection_hdl hdl_;
    std::thread io_thread_;
    std::atomic<bool> closed_{ false };

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::string> messages_;
};
