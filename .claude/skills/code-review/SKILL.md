---
name: code-review
description: Checklist for hunting real correctness bugs in a diff of the Kung-Fu-Chess C++17 engine — edge cases, off-by-one, UB, exception paths, wrong state transitions, real-time-loop cost. Load when reviewing a just-completed step (reviewer). Does not cover style (see cpp-style) or design/pattern fit (see design-patterns) — this skill is correctness only.
---

# Correctness review checklist

Goal: find bugs that survive a green build — not style, not design. Read the diff line
by line against this list; report only what you can back with a concrete failure
scenario (inputs → wrong result).

## 1. Edge cases & boundaries
- Empty / full board, first / last cell, min / max time, own vs enemy piece.
- On-the-board vs off-the-board coordinates — every index into `Board` must be
  bounds-checked before use.
- The exact transition the step claims to add — does the diff actually cover it, or
  only the happy path?

## 2. Off-by-one & arithmetic
- Loop bounds (`<` vs `<=`), pixel → cell conversion, time-to-cell math
  (`clock_ms_` / duration arithmetic), array/vector indices.

## 3. Undefined behavior
- Out-of-bounds access, dangling references/iterators (especially after a `vector`
  resize or a `PendingMove` is settled/erased), signed overflow, use of a moved-from
  object, uninitialized reads.

## 4. Exception safety
- Every throwing path matches `cpp-style` (`std::out_of_range` for off-board,
  `ParseError` for parse failures) — no silent swallowing, no throwing from a
  destructor, objects left valid on throw.

## 5. State transitions
- `PendingMove` / `settle_arrived_moves()` and any other queued/deferred state: check
  the diff doesn't apply a move immediately, doesn't double-settle, doesn't leave a
  piece in two cells, and handles the piece being captured/blocked while in flight.
- Verify `enum class` / `std::optional` states are all handled — no case silently
  falls through.

## 6. Real-time / hot-loop cost
This is a real-time engine — the game loop and per-click/per-frame paths must not
regress:
- No `new`/heap allocation, no `std::vector`/`std::string` construction, and no
  container growth inside the per-frame loop or a per-click handler on the hot path.
  Reused/reserved buffers or stack-local fixed-size data are fine; allocating fresh
  containers per frame/click is not.
- No unnecessary copies of `Board` or other large value types on the hot path — check
  for missing `const&`/`std::move` per `cpp-style`.
- Flag this even if functionally correct — it's a correctness-adjacent regression in a
  real-time engine and belongs in the report, not a silent pass.

## 7. Test-vs-diff coverage
- Cross-check the implementer's tests against the architect's test list (from
  `feature-workflow`): every listed behavior has a matching `TEST_CASE`, and no test
  silently disappeared or was weakened (assertion loosened, case deleted) to make the
  build pass.
- Confirm tests assert on public-API observable output, not private state.

## How to use this checklist
Walk the diff once per section above — don't just skim for "does it look plausible."
Only report a finding once you can state: file:line, the one-sentence defect, and a
concrete input that produces the wrong result. If nothing survives that bar, say the
diff is clean rather than padding the report with stylistic nitpicks (those belong to
`cpp-style`) or design opinions (those belong to `design-patterns`).
