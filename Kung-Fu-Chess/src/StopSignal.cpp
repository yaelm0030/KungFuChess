#include "StopSignal.h"

#ifdef _WIN32

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

namespace {

// Assumes wait_for_stop_signal() is called at most once per process (true today:
// each process runs exactly one of run_shard()/run_gateway()) — the flag is never
// reset, so a second call would return immediately without waiting for a new signal.
std::atomic<bool> g_stop_requested{false};

extern "C" void request_stop(int /*signal*/) {
    g_stop_requested = true;
}

} // namespace

void block_stop_signals() {
}

void wait_for_stop_signal() {
    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);
    while (!g_stop_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

#else

#include <csignal>

namespace {

// SIGINT and SIGTERM are the only signals this process stops on; kept as a single
// source of truth for both blocking and waiting.
sigset_t stop_signal_set() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    return set;
}

} // namespace

void block_stop_signals() {
    sigset_t set = stop_signal_set();
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
}

void wait_for_stop_signal() {
    sigset_t set = stop_signal_set();
    int received_signal = 0;
    sigwait(&set, &received_signal);
}

#endif
