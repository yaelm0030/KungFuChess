#include "ThirdParty/doctest.h"

#include "RuleEngine.h"

TEST_SUITE("RuleEngine::captures_own_color") {

TEST_CASE("a same-color destination is a self-capture") {
    Board board(2, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(1, 0, Cell{ Color::w, PieceType::N });
    CHECK(RuleEngine::captures_own_color(0, 0, 1, 0, board));
}

TEST_CASE("an enemy destination is not a self-capture") {
    Board board(2, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(1, 0, Cell{ Color::b, PieceType::N });
    CHECK_FALSE(RuleEngine::captures_own_color(0, 0, 1, 0, board));
}

TEST_CASE("an empty destination is not a self-capture") {
    Board board(2, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    CHECK_FALSE(RuleEngine::captures_own_color(0, 0, 1, 0, board));
}

TEST_CASE("an empty start cell is not a self-capture") {
    Board board(2, 1);
    board.place_at(1, 0, Cell{ Color::w, PieceType::N });
    CHECK_FALSE(RuleEngine::captures_own_color(0, 0, 1, 0, board));
}

}

TEST_SUITE("RuleEngine::is_path_clear") {

TEST_CASE("an unobstructed straight path is clear") {
    Board board(4, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    CHECK(RuleEngine::is_path_clear(0, 0, 3, 0, board));
}

TEST_CASE("a piece strictly between start and destination blocks the path") {
    Board board(4, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(2, 0, Cell{ Color::b, PieceType::N });
    CHECK_FALSE(RuleEngine::is_path_clear(0, 0, 3, 0, board));
}

TEST_CASE("a piece on the destination itself does not block the path") {
    Board board(2, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(1, 0, Cell{ Color::b, PieceType::N });
    CHECK(RuleEngine::is_path_clear(0, 0, 1, 0, board));
}

TEST_CASE("an unobstructed diagonal path is clear") {
    Board board(3, 3);
    board.place_at(0, 0, Cell{ Color::w, PieceType::B });
    CHECK(RuleEngine::is_path_clear(0, 0, 2, 2, board));
}

TEST_CASE("a piece strictly between start and destination blocks a diagonal path") {
    Board board(3, 3);
    board.place_at(0, 0, Cell{ Color::w, PieceType::B });
    board.place_at(1, 1, Cell{ Color::b, PieceType::N });
    CHECK_FALSE(RuleEngine::is_path_clear(0, 0, 2, 2, board));
}

}
