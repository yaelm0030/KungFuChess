---
name: feature-workflow
description: The small-step feature workflow for Kung-Fu-Chess — how to slice work into tiny independent UNITs and the per-step commit cycle (feat → test → fix → refactor), plus the build/test commands. Load when planning a slice (architect) or executing a step (implementer).
---

# Feature workflow — tiny steps, tight commit cycle

## Slicing philosophy

Every step is ONE small, self-contained UNIT that builds and tests on its own. Never a
big change at once. Prefer a chain of tiny steps, e.g.:

`parse board → text (IO)` → `basic click command` → `movement condition on click` →
`piece X` → `piece Y` → `BLOCKERS check` → …

If a step feels like it touches many concerns, it's too big — split it.

## Test ownership (design ≠ writing ≠ audit)

To avoid "testing only what I built" bias without adding an agent, three roles split it:

- **architect** decides *which* cases (the edge cases of this step) — before any code.
- **implementer** *writes* exactly those cases as doctest tests — no inventing/dropping.
- **reviewer** *audits* coverage independently — flags a missing edge case or bloat.

## Per-step commit cycle — gated by Brachi (strict order)

The test list is **defined up front by the architect**, before development. Then each
phase is a **gate**: the implementer does ONLY that phase's work, builds/runs tests, and
hands back; the **team lead announces the gate and asks Brachi to commit**; Brachi
commits himself; then the next phase begins. Agents do **not** commit — Brachi does.

Order of gates:

1. **Develop** the step → team lead: "Development done, build green. Commit `feat: <what>`."
2. **Write the pre-defined focused tests** (via `doctest-tests`) → "Tests done. Commit `test: <what>`."
3. **Fix** bugs the tests reveal (only if any) → "Bugs fixed, tests green. Commit `fix: <what>`."
4. **Refactor** for clarity, no behavior change (only if worthwhile) → "Refactor done. Commit `refactor: <what>`."

Skip a gate that has no real change (say so — "no bugfix needed, skipping"). **Never ask
for a commit on a red build** — build and run the tests before announcing a gate. Keep
each phase scoped to the current step only, so each commit is clean and single-purpose.

Suggested commit message ending (Brachi's choice — the harness policy is), with the
model name filled in to match whichever Claude model actually did the work this
session (check the session/system info — don't assume):

```
Co-Authored-By: Claude <actual model name> <noreply@anthropic.com>
```

> Alternative mode (if Brachi ever prefers it): agents auto-commit each phase instead of
> gating. Default is the gated, Brachi-commits flow above.

## Build & test commands (MSVC)

This is a Visual Studio solution (`Kung-Fu-Chess.sln`, x64/Debug). Build both projects
and run the test executable from the command line:

```bash
# Build (adjust MSBuild path to the installed VS 2022 edition)
MSBuild.exe Kung-Fu-Chess.sln -p:Configuration=Debug -p:Platform=x64 -m

# Run the doctest suite
./x64/Debug/Kung-Fu-Chess.Tests.exe
```

> First run: locate the real `MSBuild.exe` (e.g. via
> `vswhere -latest -find MSBuild/**/Bin/MSBuild.exe`) and confirm the test `.exe` output
> path, then pin the exact working commands here.
