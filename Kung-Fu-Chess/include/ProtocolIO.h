#pragma once

#include <iostream>
#include <optional>

#include "Board.h"

class CommandProcessor;

class ProtocolIO {
public:
    ProtocolIO(std::istream& in, std::ostream& out);

    // Returns std::nullopt writing nothing if the "Board:" header is missing,
    // or std::nullopt writing "ERROR <code>" if the board text fails to parse.
    std::optional<Board> read_board();

    void run_commands(CommandProcessor& processor);

    std::ostream& out() const { return out_; }

private:
    std::istream& in_;
    std::ostream& out_;
};
