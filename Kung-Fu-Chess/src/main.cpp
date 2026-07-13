#include <iostream>
#include <optional>

#include "Board.h"
#include "CommandProcessor.h"
#include "Controller.h"
#include "ProtocolIO.h"

int main() {
    ProtocolIO io(std::cin, std::cout);
    std::optional<Board> board = io.read_board();
    if (!board) {
        return 0;
    }

    Controller controller(std::move(*board));
    CommandProcessor processor(controller, io.out());
    io.run_commands(processor);
}
