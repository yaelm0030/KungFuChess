---
name: doctest-tests
description: How to write focused unit tests for the Kung-Fu-Chess engine using doctest. Load before writing or reviewing tests. Covers the macros, the TEST_SUITE("Class::method") + full-sentence TEST_CASE convention, testing through the public API, file layout, and the "exactly enough, no more" rule.
---

# Writing doctest tests

Tests live in the sibling project `Kung-Fu-Chess.Tests/`, which links the engine sources
directly. The doctest header is vendored at
`Kung-Fu-Chess.Tests/include/ThirdParty/doctest.h`; the runner `main` is in
`src/TestMain.cpp` (never define another main).

## Rules

- **Exactly the cases needed** for the current step — no bloat. The architect's test list
  is the spec; implement those and stop.
- **Test through the public API** (`click`, `jump`, `wait`, `print` for the engine),
  asserting on observable output (e.g. the printed board string), not private state.
- Use the **plain macros** (not `DOCTEST_`-prefixed): `TEST_SUITE`, `TEST_CASE`,
  `SUBCASE`, `CHECK`, `CHECK_FALSE`, `REQUIRE`, `CHECK_THROWS_WITH_AS`.
- **`TEST_SUITE("Class::method")`** wraps each file — one suite per file.
- **`TEST_CASE` name = a full plain-English sentence** describing the behavior:
  `TEST_CASE("a white pawn moves upward, toward decreasing y")`.
- **`SUBCASE`** for variations that share a case (e.g. several malformed inputs).
- File-local helpers (e.g. a `board_of(engine)` that captures `print()` into a string)
  go in an anonymous namespace at the top of the file.
- Comment the pixel math when it isn't obvious: `engine.click(250, 50); // -> cell (2,0)`.

## File layout

`Kung-Fu-Chess.Tests/src/<Subsystem>/<Thing>Tests.cpp`, mirroring the source structure:
`GameEngine/`, `Piece/`, `Parser/`. Name files `<Thing>Tests.cpp`
(e.g. `PawnTests.cpp`, `ClickTests.cpp`). A new `.cpp` test file must be added to the
`Kung-Fu-Chess.Tests.vcxproj` so it compiles.

## Shape of a test file

```cpp
#include "ThirdParty/doctest.h"

#include "GameEngine.h"

namespace {
    std::string board_of(GameEngine& engine) { /* capture print() */ }
} // namespace

TEST_SUITE("GameEngine::click") {
    TEST_CASE("a first click on an own piece selects it") {
        // arrange / act via public API / CHECK observable result
    }
}
```
