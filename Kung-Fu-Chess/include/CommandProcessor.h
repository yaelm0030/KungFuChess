#pragma once
#include <string>
class Controller;

// Adapter: parses one line of the stdin text protocol and dispatches it to
// the held Controller as a click/jump/wait/print command. Malformed or
// unknown commands are silently ignored, matching the original main.cpp
// run_command behavior.
class CommandProcessor {
public:
    explicit CommandProcessor(Controller& controller);
    void run_line(const std::string& line);
private:
    Controller& controller_;
};
