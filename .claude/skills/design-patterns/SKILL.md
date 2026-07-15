---
name: design-patterns
description: How to choose design patterns for the Kung-Fu-Chess engine without over- or under-engineering. Load when designing a slice (architect) or judging a diff's design (reviewer). Lists the project's existing patterns to stay consistent with, and a control question for any new abstraction.
---

# Right-sized design

Goal: the **simplest solution that stays clear and easy to debug**. A pattern is a tool,
not a trophy. Add structure only when it removes real complexity.

## Control question (for every new abstraction/pattern)

> "Does this add genuine clarity or reuse — or just another layer?"

If you can't name a concrete pain it removes, don't add it. Equally, if a repeated
`if/switch` on type is spreading, that's under-engineering — reach for the existing
Strategy/Factory instead of copy-pasting.

## Patterns already in the codebase — stay consistent

- **Strategy / polymorphism** — abstract `Piece` with per-type `is_available_move` /
  `has_blockers` overrides (`King`, `Queen`, …). New piece behavior fits here.
- **Flyweight + Factory** — `PieceFactory::get_piece(type)` returns stateless shared
  singletons (function-local `static`). Pieces hold no state; rules are pure functions.
- **Command-queue / deferred execution** — moves aren't applied immediately; they're
  queued as `PendingMove{start, dest, piece, arrival_ms}` and settled later by
  `wait()` / `settle_arrived_moves()`. Time-based effects (jump = `AirbornePiece`) are
  status overlays. New time-driven mechanics belong in this model.
- **Value class** — `Board` is plain bounds-checked data, no rules.
- **Stateless utility** — `Parser` is all `static`; text ⇄ Board, round-trippable.
- **Errors** — exceptions for exceptional paths (`std::out_of_range`, `ParseError` with a
  short code); `std::optional` for "absent", never sentinels.

## Smell checks

- Over: an interface with one impl, a manager/helper that only forwards, premature
  generality, config for things that never vary.
- Under: duplicated type-switches, rules leaking into `Board`, magic numbers/flags where
  an `enum class`/named constant reads clearer, immediate mutation where the deferred
  command-queue model is the established approach.
