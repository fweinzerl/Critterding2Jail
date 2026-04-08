# Critterding2Jail — Agent Context

## Code Style
- All source code and comments in English.
- Team/planning text in German.
- Suckless-style: minimal, readable, modular. Small composable units, no unnecessary abstraction.
- Validate at system boundaries only (user input, external APIs, file parsing).
- Trust verified internal data. Verify structure once at init, then assume invariants hold.
- Fail fast on missing required state (`exit(1)`), warn-and-continue only for optional components.
- Error format: `ERROR: <subsystem>: <message>`.
- Hot paths: avoid repeated expensive lookups, use local caches with explicit invalidation.
- Single ownership for mutable runtime entities. Explicit handoff for cross-thread transfer.
- Mutually exclusive runtime plugins must not run together; enforce at startup with hard error.
- Keep diffs focused. Incremental changes over broad rewrites.
- Consult user before architecture changes with ongoing runtime overhead (present 2-3 options with tradeoffs).
- Each patch: project build + short runtime smoke test.

## Project Structure
- 3D artificial life simulation using Bullet Physics, OpenGL, Qt6.
- Plugin-based architecture (`be_plugin_*`).
- Brain: spiking neural network (`be_plugin_brainz`), being supplemented by Iron Maiden (feedforward, CPG-based).
- Two runtime modes: single-thread (`app_critterding`) and multi-thread (`app_critterding_threads`).

## Build
- `compile-release.sh` — clean release build (`-Ofast`), deletes CMakeCache.
- `compile-debug.sh` — debug build with `-fsanitize=address`.
- TinyXML is built once as a shared library (`kernel/tinyxml/`). Plugins use the kernel copy via `"kernel/tinyxml/tinyxml.h"` — no local copies.

## Iron Maiden — Current State
- **Phase 1 (CPG) and Phase 2 (symmetric evolution) done.** See `decisions.yaml` for details.
- CPG system: `cpg_system.h/.cpp` — drives hinges, manages symmetric CPG and body evolution.
- When CPG enabled: brain creation, vision system, and lifetime learning are skipped entirely.
- Vision system disabled in `plugin.cpp` (commented out) for CPG phases.

## Bullet Hinge Motor — Key Insight
- `enableAngularMotor(enable, targetVelocity, maxMotorImpulse)` is **persistent** — keeps running until explicitly changed/disabled.
- Constraint `process()` now uses P-controller: input = target angle, velocity = error * gain, fixed max impulse.
- Hinge limits enforce range — no need for bidirectional sign filter with P-controller.
- Old approach (input as force, fixed velocity) caused gravity-drift. Don't revert to that.

## Planning Documents
- `ROADMAP.md` — Long-term vision (predator-prey ecosystem) + observability/tooling tasks.
- `PLAN_IRON_MAIDEN.md` — Brain/evolution/emergence plan (CPG, vision compression, minimal brain, phased bootstrapping).
- `decisions.yaml` — Decision DAG tracking feature status.
- Superseded: `PLAN_JAIL_BRAIN.md`, `TODO.md`, `code-style.md` (from `ai_codex` branch).
