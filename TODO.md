# TODO

## Unify Critterding Control Panels

### Why this exists
There are currently two separate implementations of `CdControlPanel`:

- `src/plugins/be_plugin_app_critterding/control_panel.*` (single-thread runtime)
- `src/plugins/be_plugin_app_critterding_threads/control_panel.*` (multi-thread runtime)

Both panels now expose similar UX goals (settings vs read-only statistics, sim speed control, and `sim steps / sec`), but the code is still duplicated.

### Why duplication happened
The thread version has a different data topology:

- single-thread panel reads directly from one `Critterding/critter_system`, `food_system`, `brain_system`
- thread panel must aggregate across multiple `thread/*/Critterding/*_system`

Because of this, the thread panel collects reference lists first and computes totals over those lists.

### Target architecture for a future cleanup
Create one shared control-panel implementation with a tiny data adapter layer:

1. Introduce a provider interface/struct (or minimal adapter entity) that exposes:
   - editable settings handles
   - read-only metric handles
   - aggregation semantics (`sum`, `avg`, `direct`)
2. Implement two providers:
   - `SingleThreadProvider`
   - `ThreadsProvider`
3. Keep one UI builder and one process/update loop.

### Acceptance criteria
- One UI code path for layout and live updates.
- Provider layer is the only place aware of single-thread vs multi-thread tree layout.
- Both start modes still work:
  - `./src/bengine app_critterding`
  - `./src/bengine app_critterding_threads`
- Existing features remain:
  - editable settings fields
  - read-only statistics
  - `sim steps / sec`
  - births/deaths counters

### Notes for future agent
- Do not assume both runtimes expose identical entity paths.
- Validate in both modes after refactor.
- Keep diffs focused: first introduce provider abstraction, then swap panel code.

## Show Life / Death Graph in Life Stat Panel

### Scope (MVP)
- Add lightweight time-series charts to `CdLifeStatsPanel` (read-only, no simulation controls).
- First iteration should show only the most useful trends:
  - `population`: critters + food
  - `events`: critter births/deaths rate (per second)

### Implementation notes
- Reuse existing `QwtPlot`/`QwtCurve` integration style from historical control-panel code.
- Keep fixed-size history buffers (ring buffer), e.g. last `300` samples.
- Update graphs at panel refresh rate (`setFps(2)`), not every simulation tick.
- For threaded mode, continue using aggregated values across all thread-local systems.
- Keep data collection logic in panel code simple and explicit; avoid introducing heavy abstractions for MVP.

### Nice-to-have (after MVP)
- Optional smoothing toggle (`raw` vs moving average).
- Additional trend lines:
  - avg adam distance
  - total critter energy / total food energy
- Optional auto-scale / fixed-scale switch for better visual comparison.

### Risks
- Too many curves make the panel unreadable; prefer 2-4 curves max initially.
- Different update cadences can make event-rate metrics noisy if sampling is too sparse.
- Threaded aggregation must be validated carefully to avoid mismatched counts.

### Acceptance criteria
- `F4` opens without crashes in both runtimes:
  - `./src/bengine app_critterding`
  - `./src/bengine app_critterding_threads`
- Graphs update continuously while simulation runs.
- No writable controls appear in `Life Stats` panel.
- Build remains green (`make -j4`).

### Suggested first patch size
- One focused patch: add 2 charts + ring buffers + per-second rate computation.
- No refactor of panel architecture in the same patch.

## Lifetime Learning Roadmap (Current State)

### Implemented baseline
- Reverted the temporary Q-learning branch and kept the existing brain architecture.
- Added lightweight lifetime adaptation by mutating brain parameters in episodes.
- Added oscillator-driven motor context:
  - per-critter `oscillator/frequency` and `oscillator/phase`
  - brain inputs `osc_sin` and `osc_cos`
  - inherited frequency with slight mutation at reproduction
- Increased default learning episode length to better match oscillator dynamics (`learning_episode_ticks = 300`).

### Next steps
- Add observability for oscillator and lifetime adaptation quality in `Life Stats`:
  - avg distance gain / time
  - episode reward trend
  - oscillator frequency distribution
- Tune oscillator defaults and mutation amplitude against stability:
  - reduce pure jitter
  - preserve exploratory variation
- Revisit optional structured controllers only after baseline locomotion is consistently better than random twitching.

### Non-goals (for now)
- No NEAT/topology evolution in this step.
- No hierarchical policy stack rewrite yet.
- No predator-specific reward shaping yet.
