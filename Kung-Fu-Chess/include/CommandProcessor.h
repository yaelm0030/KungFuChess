#pragma once
#include <iostream>
#include <string>
class Controller;

// Silently ignores malformed or unknown commands.
class CommandProcessor {
public:
    CommandProcessor(Controller& controller, std::ostream& out);
    void run_line(const std::string& line);
private:
    Controller& controller_;
    std::ostream& out_;
};