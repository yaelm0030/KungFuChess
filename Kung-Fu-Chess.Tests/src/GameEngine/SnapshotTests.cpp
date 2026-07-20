#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Constants.h"
#include "GameEngine.h"
#include "Parser.h"
#include "Position.h"

TEST_SUITE("GameEngine::snapshot") {

TEST_CASE("board_width, board_height, and is_game_over reflect the engine") {
    Board board(3, 5);
    GameEngine engine(std::move(board));

    GameSnapshot snap = engine.snapshot();

    CHECK(snap.board_width == 3);
    CHECK(snap.board_height == 5);
    CHECK_FALSE(snap.is_game_over);
}

TEST_CASE("an empty board has no pieces in the snapshot") {
    Board board(2, 2);
    GameEngine engine(std::move(board));

    CHECK(engine.snapshot().pieces.empty());
}

TEST_CASE("an idle piece appears with its type, color, pixel position, and idle state") {
    Board board(2, 2);
    board.place_at(1, 0, Cell{ Color::w, PieceType::K });
    GameEngine engine(std::move(board));

    GameSnapshot snap = engine.snapshot();

    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].type == PieceType::K);
    CHECK(snap.pieces[0].color == Color::w);
    CHECK(snap.pieces[0].pixels_location.x == constants::kCellSizePx); // cell (1, 0)
    CHECK(snap.pieces[0].pixels_location.y == 0);
    CHECK(snap.pieces[0].state == PieceState::idle);
}

TEST_CASE("a piece in flight is reported at its origin with move state") {
    Board board(3, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }));

    GameSnapshot snap = engine.snapshot();

    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].pixels_location.x == 0);
    CHECK(snap.pieces[0].pixels_location.y == 0);
    CHECK(snap.pieces[0].state == PieceState::move);
}

TEST_CASE("a piece in flight also reports its destination and travel progress") {
    Board board(3, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    GameEngine engine(std::move(board), 1000); // 1000ms/cell
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 })); // arrives at 2000ms

    engine.wait(500); // a quarter of the way there

    GameSnapshot snap = engine.snapshot();

    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].target_pixels_location.x == 2 * constants::kCellSizePx);
    CHECK(snap.pieces[0].target_pixels_location.y == 0);
    CHECK(snap.pieces[0].progress == doctest::Approx(0.25));
}

TEST_CASE("a piece that isn't moving reports its target equal to its own cell and progress of 1.0") {
    Board board(2, 2);
    board.place_at(1, 0, Cell{ Color::w, PieceType::K });
    GameEngine engine(std::move(board));

    GameSnapshot snap = engine.snapshot();

    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].target_pixels_location.x == snap.pieces[0].pixels_location.x);
    CHECK(snap.pieces[0].target_pixels_location.y == snap.pieces[0].pixels_location.y);
    CHECK(snap.pieces[0].progress == doctest::Approx(1.0));
}

TEST_CASE("a piece that just arrived is reported at its destination with short_rest state") {
    Board board(3, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }));

    engine.wait(2000); // arrival_ms for a 2-cell move at 1000ms/cell

    GameSnapshot snap = engine.snapshot();

    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].pixels_location.x == 2 * constants::kCellSizePx);
    CHECK(snap.pieces[0].pixels_location.y == 0);
    CHECK(snap.pieces[0].state == PieceState::short_rest);
}

TEST_CASE("a piece is idle again once its cooldown has expired") {
    Board board(3, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 }));

    engine.wait(2000); // arrives and stamps cooldown_end_ms = 2000 + kCooldownMs
    engine.wait(static_cast<int>(GameEngine::kCooldownMs)); // cooldown now expired

    GameSnapshot snap = engine.snapshot();

    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].state == PieceState::idle);
}

TEST_CASE("an airborne piece is reported at its cell with jump state") {
    Board board(1, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::K });
    GameEngine engine(std::move(board));

    REQUIRE(engine.request_jump(Position{ 0, 0 }));

    GameSnapshot snap = engine.snapshot();

    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].state == PieceState::jump);
}

TEST_CASE("a piece is idle again once its jump has landed") {
    Board board(1, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::K });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_jump(Position{ 0, 0 }));

    engine.wait(static_cast<int>(GameEngine::kJumpDurationMs));

    GameSnapshot snap = engine.snapshot();

    REQUIRE(snap.pieces.size() == 1);
    CHECK(snap.pieces[0].state == PieceState::idle);
}

TEST_CASE("is_game_over becomes true once a king is captured") {
    Board board(2, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(1, 0, Cell{ Color::b, PieceType::K });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 1, 0 }));

    engine.wait(1000); // arrival_ms for a 1-cell move

    CHECK(engine.snapshot().is_game_over);
}

TEST_CASE("score_w and score_b start at zero") {
    Board board(2, 2);
    GameEngine engine(std::move(board));

    GameSnapshot snap = engine.snapshot();

    CHECK(snap.score_w == 0);
    CHECK(snap.score_b == 0);
}

TEST_CASE("a normal capture credits the capturing color's score with the captured piece's value") {
    Board board(3, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(2, 0, Cell{ Color::b, PieceType::N });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 2, 0 })); // wR captures bN

    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);

    GameSnapshot snap = engine.snapshot();
    CHECK(snap.score_w == 3); // knight
    CHECK(snap.score_b == 0);
}

TEST_CASE("a hostile mid-flight collision credits the winning side's score with the loser's value") {
    Board board(4, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::R });
    board.place_at(3, 0, Cell{ Color::b, PieceType::Q });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 3, 0 })); // wR scheduled first
    REQUIRE(engine.request_move(Position{ 3, 0 }, Position{ 0, 0 })); // bQ scheduled second, loses the head-on collision

    engine.wait(2 * GameEngine::kDefaultMoveMsPerCell);

    GameSnapshot snap = engine.snapshot();
    CHECK(snap.score_w == 9); // queen
    CHECK(snap.score_b == 0);
}

TEST_CASE("a piece captured by a still-airborne guard credits the guard's color") {
    Board board(2, 1);
    board.place_at(0, 0, Cell{ Color::w, PieceType::K });
    board.place_at(1, 0, Cell{ Color::b, PieceType::R });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_jump(Position{ 0, 0 }));                  // wK jumps, guarding its cell
    REQUIRE(engine.request_move(Position{ 1, 0 }, Position{ 0, 0 })); // bR moves onto the guarded cell

    engine.wait(GameEngine::kJumpDurationMs);

    GameSnapshot snap = engine.snapshot();
    CHECK(snap.score_w == 5); // rook
    CHECK(snap.score_b == 0);
}

TEST_CASE("a fresh engine's snapshot has an empty move_history") {
    Board board(2, 2);
    GameEngine engine(std::move(board));

    CHECK(engine.snapshot().move_history.empty());
}

TEST_CASE("the snapshot's move_history mirrors move_history() after moves are recorded, in order") {
    Board board = Parser::parse_board({
        "bR bN .",
        ".  .  .",
        "wR .  wN",
    });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_move(Position{ 0, 0 }, Position{ 0, 1 })); // bR down one cell
    REQUIRE(engine.request_move(Position{ 2, 2 }, Position{ 0, 1 })); // wN's L-shaped move

    GameSnapshot snap = engine.snapshot();

    REQUIRE(snap.move_history.size() == 2);
    for (size_t i = 0; i < snap.move_history.size(); ++i) {
        CHECK(snap.move_history[i].type == engine.move_history()[i].type);
        CHECK(snap.move_history[i].color == engine.move_history()[i].color);
        CHECK(snap.move_history[i].source == engine.move_history()[i].source);
        CHECK(snap.move_history[i].destination == engine.move_history()[i].destination);
    }
}

TEST_CASE("a pawn promotion does not affect the score") {
    Board board(1, 2);
    board.place_at(0, 1, Cell{ Color::w, PieceType::P });
    GameEngine engine(std::move(board));
    REQUIRE(engine.request_move(Position{ 0, 1 }, Position{ 0, 0 })); // promotes to Q on arrival, no capture

    engine.wait(GameEngine::kDefaultMoveMsPerCell);

    GameSnapshot snap = engine.snapshot();
    CHECK(snap.score_w == 0);
    CHECK(snap.score_b == 0);
}

} // TEST_SUITE
