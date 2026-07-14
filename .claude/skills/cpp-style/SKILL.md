---
name: cpp-style
description: The C++17 coding conventions and senior quality bar for the Kung-Fu-Chess engine. Load BEFORE writing or reviewing any C++ code — covers naming (member vars end with a trailing underscore, NOT m_), formatting, comment style, and the correctness/ownership bar. Keeps code concise, clear, and debuggable.
---

# Kung-Fu-Chess C++ style & quality

Write code that reads like the code already in `Kung-Fu-Chess/src` and `include`.
Concise, clear, debuggable. No cleverness for its own sake.

## Part A — Senior quality bar (correctness first)

- **RAII & ownership**: resources managed by their owning object; ownership is obvious.
- **const-correctness**: `const` on methods that don't mutate, on locals that don't
  change, on reference params you only read (`const Board&`).
- **Exception safety**: throw for exceptional paths only (e.g. off-board → `std::out_of_range`;
  parse errors → `ParseError`). Leave objects in a valid state on throw.
- **Zero UB**: bounds-checked access, no dangling refs/iterators, no signed overflow reliance.
- **Values over sentinels**: `std::optional<Cell>` for "maybe a piece", `enum class` for
  fixed sets — never magic ints or bool flags where an enum reads clearer.
- **No needless copies**: pass big types by `const&`; `std::move` when transferring.
- **Small pure functions**: movement rules etc. are pure functions of (args, board).
- **Function length**: as short as the logic honestly allows, but never at the cost of
  clarity — don't fragment a function into unlabeled pieces just to hit a line count, and
  don't cram unrelated steps into one function just to avoid a second one. One function,
  one job, one level of abstraction: if you're reaching for a comment to mark off a
  section (`// now validate`, `// now apply`), that's usually a sign it should be its own
  well-named function instead. Extract only when the extracted piece is independently
  understandable (has a name that fully explains it) — extracting a fragment that only
  makes sense read inline just moves confusion around.
- **Zero compiler warnings.**

## Part B — Naming & formatting (match existing code exactly)

- **Member variables**: **trailing underscore** — `board_`, `clock_ms_`, `selected_`,
  `pending_moves_`. NOT `m_`. (Plain data structs like `Board`'s `width`/`height`/`cells`
  stay bare — follow the neighbor.)
- **Functions / methods / locals / params**: `snake_case` — `is_available_move`,
  `settle_arrived_moves`, `start_x`, `dest_cell`. No `tmp`, `x2`, `foo` — names say intent.
- **Types / classes / structs**: `PascalCase` — `GameEngine`, `PieceFactory`, `PendingMove`.
- **Constants**: `k`-prefixed on `static constexpr` — `kDefaultMoveMsPerCell`, `kJumpDurationMs`.
- **Enums**: `enum class`, terse enumerators matching text tokens — `Color { w, b }`,
  `PieceType { K, Q, R, B, N, P }`.
- **Getters**: Board uses `get_`-prefix (`get_at`, `get_width`); GameEngine uses bare
  nouns (`clock_ms()`, `game_over()`). Follow the class you're in.
- **Files**: `PascalCase` = primary class. Headers in `include/`, sources in `src/`.
- **Formatting**: 4 spaces, no tabs. K&R braces (opening brace on the same line). Braces
  even around single-statement `if` bodies. Braced-init with inner spaces: `Cell{ Color::w, PieceType::P }`.
- **`#include` order**: own header → blank → `<std>` headers → blank → project headers.
  File-local helpers go in an anonymous `namespace { ... } // namespace`.

## Part C — Comments

`//` line comments only (no `/* */`, no Doxygen). Comment the **why / contract**, not a
line-by-line narration. Every public method gets a short contract comment (behavior +
edge cases). Add an inline note only to justify a non-obvious line
(`// white moves up (-y), black moves down (+y)`).
