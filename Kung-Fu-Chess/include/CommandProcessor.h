#pragma once
#include <iostream>
#include <string>
class Controller;

// Adapter: parses one line of the stdin text protocol and dispatches it to
// the held Controller as a click/jump/wait/print command. Malformed or
// unknown commands are silently ignored, matching the original main.cpp
// run_command behavior. The "print board" command writes to the
// caller-supplied out_, so it shares whichever stream ProtocolIO was
// constructed with.
class CommandProcessor {
public:
    CommandProcessor(Controller& controller, std::ostream& out);
    void run_line(const std::string& line);
private:
    Controller& controller_;
    std::ostream& out_;
};
