#ifndef WEBSOCKETPP_COMMON_IO_SERVICE_COMPAT_HPP
#define WEBSOCKETPP_COMMON_IO_SERVICE_COMPAT_HPP

// websocketpp 0.8.2's asio transport was written against the pre-1.13 Asio
// API (asio::io_service, asio::io_service::strand, and
// steady_timer::expires_from_now()). Modern standalone Asio (this project
// vendors 1.38.1) dropped all three in favor of io_context + the executor
// model and expiry()/expires_after(). This shim reintroduces just the
// surface websocketpp's transport/asio actually calls (see grep of
// transport/asio/{connection,endpoint,security}.hpp): default construction,
// run/run_one/stop/poll/poll_one/stopped (inherited unchanged from
// io_context), post(handler), nested work/strand RAII helpers, and
// steady_timer::expires_from_now() as a thin wrapper over expiry().
// (resolver::query/::iterator have no equivalent shim here -- those call
// sites in endpoint.hpp were edited directly to the modern
// resolve()/results_type API instead, since basic_resolver's structural
// changes don't reduce to a small wrapper.)

#include <asio.hpp>
#include <asio/strand.hpp>

#include <utility>

namespace websocketpp {
namespace lib {
namespace asio {

class io_service : public ::asio::io_context {
public:
    io_service() : ::asio::io_context() {}

    template <typename Handler>
    void post(Handler&& handler) {
        ::asio::post(*this, std::forward<Handler>(handler));
    }

    // Keeps run()/run_one() from returning while held, matching the old
    // io_service::work RAII guard websocketpp constructs and later resets.
    class work {
    public:
        explicit work(io_service& ios) : guard_(::asio::make_work_guard(ios)) {}

    private:
        ::asio::executor_work_guard<::asio::io_context::executor_type> guard_;
    };

    class strand {
    public:
        explicit strand(io_service& ios) : strand_(ios.get_executor()) {}

        template <typename Handler>
        auto wrap(Handler handler) {
            return ::asio::bind_executor(strand_, std::move(handler));
        }

    private:
        ::asio::strand<::asio::io_context::executor_type> strand_;
    };
};

class steady_timer : public ::asio::steady_timer {
public:
    using ::asio::steady_timer::steady_timer;

    // Old Asio's setter/getter pair; websocketpp only ever calls the no-arg
    // getter form to check how much time is left before firing.
    duration expires_from_now() const {
        return expiry() - clock_type::now();
    }
};

} // namespace asio
} // namespace lib
} // namespace websocketpp

#endif // WEBSOCKETPP_COMMON_IO_SERVICE_COMPAT_HPP
