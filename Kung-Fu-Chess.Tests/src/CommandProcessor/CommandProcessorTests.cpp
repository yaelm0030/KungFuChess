#include "ThirdParty/doctest.h"

#include <iostream>
#include <sstream>
#include <string>

#include "CommandProcessor.h"
#include "Controller.h"
#include "GameEngine.h"
#include "Parser.h"

namespace {

Board make_board() {
    return Parser::parse_board({
        "bR bN .",
        ".  .  .",
        "wR .  wN",
    });
}

std::string board_of(const Controller& controller) {
    std::ostringstream oss;
    controller.print(oss);
    return oss.str();
}

// Redirects std::cout into an ostringstream for the duration of the call,
// restoring the original stream buffer afterward even if run_line throws.
std::string captured_stdout(CommandProcessor& processor, const std::string& line) {
    std::ostringstream captured;
    std::streambuf* original = std::cout.rdbuf(captured.rdbuf());
    try {
        processor.run_line(line);
    } catch (...) {
        std::cout.rdbuf(original);
        throw;
    }
    std::cout.rdbuf(original);
    return captured.str();
}

} // namespace

TEST_SUITE("CommandProcessor") {

TEST_CASE("a click line dispatches to Controller::click and selects the clicked piece") {
    Controller controller(make_board());
    CommandProcessor processor(controller);

    processor.run_line("click 50 50");
    REQUIRE(controller.has_selection());
    CHECK(controller.selected()->x == 0);
    CHECK(controller.selected()->y == 0);
}

TEST_CASE("a jump line dispatches to Controller::jump and can clear the selection") {
    Controller controller(make_board());
    CommandProcessor processor(controller);

    processor.run_line("click 50 50");
    REQUIRE(controller.has_selection());

    processor.run_line("jump 50 50");
    CHECK_FALSE(controller.has_selection());
}

TEST_CASE("a wait line dispatches to Controller::wait and settles an in-flight move") {
    Controller controller(make_board());
    CommandProcessor processor(controller);

    processor.run_line("click 50 50");  // select bR at (0,0)
    processor.run_line("click 50 150"); // move down to (0,1)

    processor.run_line("wait " + std::to_string(GameEngine::kDefaultMoveMsPerCell));
    CHECK(board_of(controller) == ". bN .\nbR . .\nwR . wN\n");
}

TEST_CASE("a print line dispatches to Controller::print and writes the board to std::cout") {
    Controller controller(make_board());
    CommandProcessor processor(controller);

    std::string printed = captured_stdout(processor, "print board");
    CHECK(printed == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("malformed numeric arguments are ignored") {
    Controller controller(make_board());
    CommandProcessor processor(controller);

    SUBCASE("non-numeric x") {
        processor.run_line("click a 50");
    }
    SUBCASE("non-numeric y") {
        processor.run_line("click 50 a");
    }
    CHECK_FALSE(controller.has_selection());
}

TEST_CASE("an unrecognized command name is ignored") {
    Controller controller(make_board());
    CommandProcessor processor(controller);

    processor.run_line("foo 1 2");
    CHECK_FALSE(controller.has_selection());
    CHECK(board_of(controller) == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("a blank line is ignored") {
    Controller controller(make_board());
    CommandProcessor processor(controller);

    processor.run_line("");
    CHECK_FALSE(controller.has_selection());
}

TEST_CASE("a command with the wrong number of arguments is ignored") {
    Controller controller(make_board());
    CommandProcessor processor(controller);

    SUBCASE("click missing an argument") {
        processor.run_line("click 50");
        CHECK_FALSE(controller.has_selection());
    }
}

TEST_CASE("print with an unrecognized second token is ignored") {
    Controller controller(make_board());
    CommandProcessor processor(controller);

    std::string printed = captured_stdout(processor, "print foo");
    CHECK(printed.empty());
}

}
