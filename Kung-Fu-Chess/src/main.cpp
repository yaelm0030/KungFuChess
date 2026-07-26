#include <iostream>
#include <optional>
#include <string>

#include "Board.h"
#include "CommandProcessor.h"
#include "Controller.h"
#include "GameServer.h"
#include "PqxxSmokeCheck.h"
#include "ProtocolIO.h"

namespace {

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

    ProtocolIO io(std::cin, std::cout);
    std::optional<Board> board = io.read_board();
    if (!board) {
        return 0;
    }

    Controller controller(std::move(*board));
    CommandProcessor processor(controller, io.out());
    io.run_commands(processor);
}
