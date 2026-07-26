#include <iostream>
#include <optional>
#include <string>

#include "Board.h"
#include "CommandProcessor.h"
#include "Controller.h"
#include "GameServer.h"
#include "PostgresUserRepository.h"
#include "PqxxSmokeCheck.h"
#include "ProtocolIO.h"

namespace {

// Exercises PostgresUserRepository against the docker-compose Postgres:
// ensure_user on a fixed username twice, proving the second call returns the
// same rating as the first (upsert doesn't reset it). Reuses one fixed
// username across runs so repeated invocations upsert the same row instead
// of accumulating new ones.
int run_user_repo_smoke_check() {
    try {
        const std::string username = "__user_repo_smoke_check__";

        PostgresUserRepository repository(kDatabaseConnectionString);

        int first = repository.ensure_user(username);
        std::cout << "ensure_user(\"" << username << "\") first call: " << first << "\n";

        int second = repository.ensure_user(username);
        std::cout << "ensure_user(\"" << username << "\") second call: " << second << "\n";

        if (first != second) {
            std::cout << "user repo smoke check FAILED: rating changed between calls\n";
            return 1;
        }

        std::cout << "user repo smoke check OK\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "user repo smoke check FAILED: " << e.what() << "\n";
        return 1;
    }
}

// Reads a board from stdin and serves it over WebSocket on port, blocking
// until the user presses Enter. Same board-loading path as the stdin/stdout
// protocol below, just handed to a GameServer instead of a Controller.
int run_server(uint16_t port) {
    ProtocolIO io(std::cin, std::cout);
    std::optional<Board> board = io.read_board();
    if (!board) {
        return 0;
    }

    GameServer server(std::move(*board));
    server.start(port);
    std::cout << "Server listening on port " << server.port() << ". Press Enter to stop.\n";

    std::string discard;
    std::getline(std::cin, discard);

    server.stop();
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "--serve") {
        return run_server(static_cast<uint16_t>(std::stoi(argv[2])));
    }
    if (argc >= 2 && std::string(argv[1]) == "--pqxx-smoke-check") {
        return run_pqxx_smoke_check();
    }
    if (argc >= 2 && std::string(argv[1]) == "--user-repo-smoke-check") {
        return run_user_repo_smoke_check();
    }

    ProtocolIO io(std::cin, std::cout);
    std::optional<Board> board = io.read_board();
    if (!board) {
        return 0;
    }

    Controller controller(std::move(*board));
    CommandProcessor processor(controller, io.out());
    io.run_commands(processor);
}
