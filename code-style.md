# Code Style (v2)

## Language
- All source code must be written in English.
- All code comments must be written in English.
- Team/user-facing conversation and planning text should be in German.

## Core principles
- Follow a suckless-style approach: minimal, readable, modular.
- Prefer small, composable units over large abstractions.
- Avoid unnecessary architecture and indirection.
- Keep code straightforward and explicit.

## Implementation style
- Favor simple functional transformations when they improve readability (`map` / `filter` / `reduce` style where appropriate).
- Prefer one short goal-comment above non-obvious blocks.
- Avoid comments that restate obvious code behavior.
- Add thorough documentation only for truly intricate logic.

## Validation, invariants, lifecycle
- Validate at system boundaries only:
  - user input
  - external APIs/services
  - file system access and file parsing
- Trust verified self-authored internal data (for example project-owned JSON config).
- Verify required internal structure once at initialization, then assume invariants hold.
- Keep initialization and publication explicit: `build -> verify -> publish`.
- Do not add repetitive runtime guards for already-verified internal state.
- Do not silently skip required entities/components in runtime loops (`continue`/soft fallback).
- If a required component or invariant is missing/invalid, fail fast and stop.
- Use warnings and continue only for explicitly optional components.

## Error and logging policy
- Use a clear and consistent error format: `ERROR: <subsystem>: <message>`.
- On required-state failure, terminate immediately (`exit(1)`).

## Concurrency and plugin boundaries
- Define single ownership for mutable runtime entities at every point in time.
- For cross-thread transfer/migration, use explicit handoff phases and avoid concurrent access windows.
- Do not run mutually exclusive runtime plugins together.
- Enforce plugin exclusivity at startup with a hard error and immediate exit.

## Performance and caches
- In hot paths (especially per-frame/per-tick `process()` loops), avoid repeated expensive lookups/casts when a local shortcut/cache is possible.
- Any cache must have an explicit invalidation rule and ownership model.

## Change discipline
- Keep diffs focused and avoid unrelated refactors.
- Prefer incremental changes over broad rewrites.
- Preserve established repository patterns in legacy areas unless there is a clear and immediate benefit.
- Before architecture changes with ongoing runtime overhead, consult the user and present 2-3 options with explicit tradeoffs (`runtime cost`, `implementation effort`, `future flexibility`).
- For each patch, do at least: project build + short runtime smoke test (unless impossible; then state it explicitly).