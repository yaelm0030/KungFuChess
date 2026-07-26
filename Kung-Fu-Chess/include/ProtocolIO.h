#pragma once

#include <iostream>
#include <optional>

#include "Board.h"

class CommandProcessor;

class ProtocolIO {
public:
    ProtocolIO(std::istream& in, std::ostream& out);

    // nullopt with no output if "Board:" is missing, or with "ERROR <code>" if parsing fails.
    std::optional<Board> read_board();

    void run_commands(CommandProcessor& processor);

    std::ostream& out() const { return out_; }

private:
    std::istream& in_;
    std::ostream& out_;
};
