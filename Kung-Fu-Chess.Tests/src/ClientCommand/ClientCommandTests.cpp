#include "ThirdParty/doctest.h"

#include <sstream>
#include <string>

#include "ClientCommand.h"
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

} // namespace

TEST_SUITE("ClientCommand::apply") {

TEST_CASE("a click line selects the clicked piece") {
    Controller controller(make_board());

    ClientCommand::apply(controller, "click 50 50");

    REQUIRE(controller.has_selection());
    CHECK(controller.selected()->x == 0);
    CHECK(controller.selected()->y == 0);
}

TEST_CASE("a jump line is applied to the controller") {
    Controller controller(make_board());
    ClientCommand::apply(controller, "click 50 50");
    REQUIRE(controller.has_selection());

    ClientCommand::apply(controller, "jump 50 50");

    CHECK_FALSE(controller.has_selection());
}

TEST_CASE("a wait line is ignored") {
    Controller controller(make_board());
    ClientCommand::apply(controller, "click 50 50");   // select bR at (0,0)
    ClientCommand::apply(controller, "click 50 150");  // schedule move to (0,1)

    ClientCommand::apply(controller, "wait " + std::to_string(GameEngine::kDefaultMoveMsPerCell));

    CHECK(board_of(controller) == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("a print line is ignored") {
    Controller controller(make_board());

    ClientCommand::apply(controller, "print board");

    CHECK_FALSE(controller.has_selection());
    CHECK(board_of(controller) == Parser::board_to_string(make_board()) + "\n");
}

TEST_CASE("a click line selects the clicked piece when acting_color matches it") {
    Controller controller(make_board());

    ClientCommand::apply(controller, "click 50 250", Color::w); // white rook at (0,2)

    REQUIRE(controller.has_selection(Color::w));
    CHECK(controller.selected(Color::w)->x == 0);
    CHECK(controller.selected(Color::w)->y == 2);
}

TEST_CASE("a click line on the other color's piece is silently ignored") {
    Controller controller(make_board());

    ClientCommand::apply(controller, "click 50 50", Color::w); // black rook at (0,0)

    CHECK_FALSE(controller.has_selection(Color::w));
}

TEST_CASE("malformed input is ignored without throwing") {
    Controller controller(make_board());

    SUBCASE("wrong arg count") {
        ClientCommand::apply(controller, "click 5");
    }
    SUBCASE("non-numeric args") {
        ClientCommand::apply(controller, "click a b");
    }
    SUBCASE("unknown command") {
        ClientCommand::apply(controller, "foo 1 2");
    }
    SUBCASE("blank line") {
        ClientCommand::apply(controller, "");
    }

    CHECK_FALSE(controller.has_selection());
}

}
