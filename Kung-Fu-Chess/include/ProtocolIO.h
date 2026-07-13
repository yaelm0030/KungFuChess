#pragma once

#include <iostream>
#include <optional>

#include "Board.h"

class CommandProcessor;

// Adapter: owns all stdin/stdout access for the text protocol, so main()
// does no I/O of its own. Reads the initial "Board:" header and board text,
// then hands off the rest of the stream line-by-line to a CommandProcessor.
class ProtocolIO {
public:
    ProtocolIO(std::istream& in, std::ostream& out);

    // Reads the "Board:" header and the board lines up to a "Commands:"
    // marker (or EOF). Returns std::nullopt, writing nothing, if the header
    // is missing; returns std::nullopt, writing "ERROR <code>" to out_, if
    // the board text fails to parse.
    std::optional<Board> read_board();

    // Feeds every remaining line in in_ to processor.run_line, in order,
    // until EOF.
    void run_commands(CommandProcessor& processor);

    // The stream this ProtocolIO writes to, so callers (main, tests) can
    // construct a CommandProcessor that shares the exact same stream.
    std::ostream& out() const { return out_; }

private:
    std::istream& in_;
    std::ostream& out_;
};
