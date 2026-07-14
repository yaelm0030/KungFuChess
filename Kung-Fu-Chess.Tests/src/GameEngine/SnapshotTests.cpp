#include "ThirdParty/doctest.h"

#include "Board.h"
#include "Constants.h"
#include "GameEngine.h"
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

} // TEST_SUITE
