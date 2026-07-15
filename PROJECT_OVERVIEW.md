# Kung-Fu-Chess — Project Overview

## What this is

A chess variant where there are no turns. Both players can move any of
their pieces at any time, and moves take real time to travel across the
board instead of resolving instantly. Two moves racing toward the same
cell area can collide, land at different times, or one piece can "jump"
in place to dodge an incoming capture. It's chess, but live.

This repo is currently just the **engine** — a text-driven simulation
core with no graphics yet. It reads a starting board and a stream of
commands (`click`, `jump`, `wait`, `print board`) from stdin and prints
the resulting board state. The pixel-coordinate handling in the code is
there so a future graphical front end can plug in directly, without the
core logic caring whether input came from a mouse or a test script.

## The big picture (architecture)

Think of it as four layers, each one only knowing about the layer below it:

```
main.cpp          reads stdin, dispatches commands
     │
Controller        "what does a click/jump mean?" (UI-facing, no chess rules)
     │
GameEngine        "is this move legal, and when does it land?" (the referee)
     │
RealTimeArbiter    "the clock" — tracks in-flight moves & jumps, applies them
     │
Board              the grid of cells — pure data storage, no rules at all
```

Alongside this stack:

- **Piece / PieceFactory** — one class per chess piece (King, Queen, Rook,
  Bishop, Knight, Pawn), each knowing only its own movement shape.
- **RuleEngine** — shared helpers pieces use: "is the path blocked?" and
  "would this capture my own color?"
- **BoardMapper** — converts a pixel click into a board cell.
- **Parser** — converts the text board format to/from a `Board`.

The layering means each piece is dumb about *blocking and turns*, the
board is dumb about *chess*, and the controller is dumb about *chess
rules entirely* — it just forwards "click here" to the engine and reacts
to yes/no. This keeps each class small and independently testable (see
`Kung-Fu-Chess.Tests/`, which mirrors this same folder structure).

## The real-time twist

Normal chess engines just check "is this move legal" and apply it
immediately. This one adds a live clock:

- Every move takes `distance_in_cells × move_ms_per_cell` milliseconds
  to arrive (default 1000ms per cell).
- While a move is in flight, that piece can't be reselected or redirected.
- Two moves whose paths cross mid-flight can collide: whichever was
  scheduled first wins and is truncated to stop at the shared cell,
  while the other move's piece is removed from the board entirely.
- A piece can `jump` in place for a fixed window (1000ms). While
  airborne it can't be captured or moved — but if an enemy piece's move
  arrives on that cell during the jump, the jumper captures *it* instead.
- Time only moves forward when the `wait <ms>` command advances the
  clock; that's also the moment queued moves/jumps get resolved
  (`RealTimeArbiter::advance` → `settle_arrived_moves`).

## Classes, functions, and variables

### `Board` (`Board.h/.cpp`)
Plain grid storage — nothing about chess.
- `cells`: a flat `vector<optional<Cell>>`, indexed by `y * width + x`.
- `get_at(x, y)`, `place_at(x, y, cell)`, `clear_at(x, y)` — bounds-checked
  read/write; a `Cell` is just `{ Color, PieceType }` (see `Types.h`).

### `Parser` (`Parser.h/.cpp`)
Static-only utility, no state. Converts between the text board format
(e.g. `wK`, `bP`, `.` for empty) and a `Board`. Throws `ParseError` on
malformed input (mismatched row widths, unknown tokens).

### `Piece` + subclasses (`Piece.h/.cpp`)
Abstract interface with one method per rule:
- `is_available_move(...)` — is this specific move shape/capture legal?
- `has_blockers(...)` — is something in the way? (always `false` for
  King/Knight, since neither can be blocked.)

`King`, `Queen`, `Rook`, `Bishop`, `Knight`, `Pawn` each implement these
with their own movement shape. `Pawn` is the most involved: direction
depends on color, two-cell opening move only from its home row,
diagonal-only captures.

`PieceFactory::get_piece(type)` hands back a shared static instance per
type (flyweight pattern — pieces hold no per-instance state, so one
`Rook` object serves every rook on the board).

### `RuleEngine` (`RuleEngine.h/.cpp`)
Two static helpers shared by all pieces:
- `captures_own_color(...)` — would this move land on a friendly piece?
- `is_path_clear(...)` — walks the cells strictly between start and
  dest on a straight/diagonal line, checking for occupancy.

### `RealTimeArbiter` (`RealTimeArbiter.h/.cpp`)
The clock and scheduler. Owns two lists:
- `pending_moves_` — moves in flight, each with a `start`, `dest`,
  `piece`, and `arrival_ms`.
- `airborne_` — pieces mid-jump, each with a `cell`, `piece`, and
  `land_ms`.

Key members: `clock_ms_` (current simulated time), `move_ms_per_cell_`
(travel speed). Key methods: `schedule_move`, `start_jump`,
`advance(ms)` (ticks the clock, resolves any due mid-movement
collisions between pending moves, and settles anything that has
arrived). `settle_arrived_moves()` is the private method that actually
mutates the `Board` when a move lands — including pawn promotion and
the airborne-capture special case.

### `GameEngine` (`GameEngine.h/.cpp`)
The referee. Owns the `Board` and the `RealTimeArbiter`. Public API:
- `request_move(start, dest)` — validates against the piece's own rule
  (via `PieceFactory`/`RuleEngine`), then hands off to the arbiter,
  which schedules it even if its route crosses another in-flight move.
- `request_jump(cell)` — starts a jump if the piece is present and free.
- `wait(ms)` — advances time; sets `game_over_` if a king was captured.
- `is_selectable(cell)`, `color_at(cell)` — read-only queries the
  `Controller` uses to decide what a click means.

### `Controller` (`Controller.h/.cpp`)
Translates pixel clicks into engine calls. Knows nothing about chess
rules — only selection state (`selected_`, an `optional<Position>`).
- `click(x, y)` — first click selects a piece; a second click on a
  friendly piece re-selects it, otherwise it's treated as a move
  request; clicking off-board cancels the selection.
- `jump(x, y)` — requests a jump; drops the selection if the jumped
  piece was selected (an airborne piece can't be moved).

### `BoardMapper` (`BoardMapper.h/.cpp`)
One static function, `pixel_to_cell`, dividing pixel coordinates by
`constants::kCellSizePx` to get a board cell, or `nullopt` if outside
the board.

### `Position` / `Types.h` / `Constants.h`
Small shared value types and tunables:
- `Position { x, y }` — the coordinate type used everywhere above.
- `Color { w, b }`, `PieceType { K, Q, R, B, N, P }`, `Cell { color, type }`.
- `constants::kCellSizePx` (100), `kDefaultMoveMsPerCell` (1000),
  `kJumpDurationMs` (1000).

### `main.cpp`
Reads a `Board:`-prefixed board from stdin (via `Parser`), then a
`Commands:` section, one command per line: `click x y`, `jump x y`,
`wait ms`, `print board`. Wires up one `GameEngine` and one `Controller`
and dispatches each line to them.

## Tests

`Kung-Fu-Chess.Tests/` mirrors the source layout
(`GameEngine/`, `Controller/`, `RuleEngine/`) with one test file per
behavior area (promotion, jumping, wait/timing, printing, game-over).

## UI layer (in progress)

`Kung-Fu-Chess.UI/` (with `include/`, `src/`, and `assets/images/`) is scaffolded as a
new sibling project for a C++ graphical front end. It will render a `GameSnapshot`
handed to it by the backend — `board_width`, `board_height`, and a list of `pieces`
(`type`, `color`, `pixels_location`, `state`: idle/jump/move/etc.), plus `is_game_over`
— by drawing board and piece images, picking each piece's sprite by its type, color,
and current state. No `.vcxproj`/solution wiring or rendering library chosen yet;
that comes once implementation starts.
