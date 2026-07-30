#ifndef _WIN32

#include "ThirdParty/doctest.h"

#include <csignal>

#include "StopSignal.h"

namespace {

// Restores the thread's original signal mask on scope exit. Without this, a test that
// calls block_stop_signals() would leave SIGINT/SIGTERM blocked for the rest of the
// kfc_tests process (doctest runs all cases on one thread) with nothing left to consume
// them — a later hung test would become impossible to interrupt with Ctrl+C.
class SignalMaskGuard {
public:
    SignalMaskGuard() {
        pthread_sigmask(SIG_BLOCK, nullptr, &original_mask_);
    }
    ~SignalMaskGuard() {
        pthread_sigmask(SIG_SETMASK, &original_mask_, nullptr);
    }

private:
    sigset_t original_mask_;
};

} // namespace

TEST_SUITE("StopSignal") {

TEST_CASE("block_stop_signals blocks SIGINT and SIGTERM in the calling thread's mask") {
    SignalMaskGuard mask_guard;
    block_stop_signals();

    sigset_t mask;
    pthread_sigmask(SIG_BLOCK, nullptr, &mask);

    CHECK(sigismember(&mask, SIGINT) == 1);
    CHECK(sigismember(&mask, SIGTERM) == 1);
}

// The signal is already pending before wait_for_stop_signal() is called, so this returns
// essentially instantly; no bounded-timeout helper needed (a real regression here would
// hang the test binary rather than this one case).
TEST_CASE("wait_for_stop_signal consumes a pending SIGTERM and returns") {
    SignalMaskGuard mask_guard;
    block_stop_signals();
    raise(SIGTERM);

    wait_for_stop_signal();

    CHECK(true);
}

TEST_CASE("wait_for_stop_signal consumes a pending SIGINT and returns") {
    SignalMaskGuard mask_guard;
    block_stop_signals();
    raise(SIGINT);

    wait_for_stop_signal();

    CHECK(true);
}

}

#endif
