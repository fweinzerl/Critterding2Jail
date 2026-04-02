# Iron Maiden — Constrained Emergence Plan

## Motivation

The current system asks evolution to simultaneously invent locomotion, vision, and
feeding behaviour inside one unstructured spiking network.  That is equivalent to
asking a random number generator to write a walking controller — possible in
theory, practically never observed in reasonable time.

Key structural problems in the current code:

| Problem | Where | Impact |
|---------|-------|--------|
| 256 raw RGBA retina inputs into random synapses | `vision_system.cpp` | No spatial structure preserved → noise for the network |
| `eat` motor must fire AND collision must happen | `plugin.cpp:368-397` | AND-gate over two independently hard problems |
| "Alter Weight" replaces with full-range random value | `brain_system.cpp` mutation table | Destroys whatever the network had learned |
| Initial brain: 50-100 random neurons, random wiring | `brain_system.cpp` | Near-zero chance of producing coordinated movement |

## Design Principles

1. **Give locomotion for free** — the brain modulates, not generates.
2. **Compress vision before the brain** — fixed preprocessing, 4 signals not 256.
3. **Simplify eating initially** — contact = eat, no motor gate.
4. **Mutate gently** — small perturbations, not replacements.
5. **Bootstrap in phases** — each phase builds on a working previous stage.
6. **Exploit symmetry** — mirror body and CPG amplitudes to halve the search space.

---

## Phase 1 — Validate CPG (DONE)

- CPG drives all hinges, brain disconnected.
- Eating automatic on contact (no `eat` motor needed).
- **Result**: Critters walk with stable periodic movement.
- **Key insight**: Hinge motor needs P-controller (target angle, not force/velocity)
  to avoid gravity-drift.  See CLAUDE.md for details.

---

## Phase 2 — Evolve Locomotion

### Goal
Evolution discovers efficient locomotion by mutating CPG and body parameters.
No brain, no vision, no reward function — natural selection through the
existing energy/food/reproduction loop.

### Symmetry

Left/right body symmetry enforced:
- Body: right side = mirror of left side (halves body parameters).
- CPG amplitudes: right = left (halves amplitude parameters).
- CPG phase offset between sides: single evolvable parameter
  (0 = hopping, π = alternating gait — let evolution decide).

### Evolvable Parameters

| Category | Parameter | Mutation |
|----------|-----------|----------|
| CPG | `frequency` | ±small delta |
| CPG | `shoulder_amplitude` (both sides) | ±small delta |
| CPG | `elbow_amplitude` (both sides) | ±small delta |
| CPG | `elbow_phase` (relative to shoulder) | ±small delta |
| CPG | `side_phase_offset` (left vs right) | ±small delta |
| Body | symmetric body part dimensions | ±small delta |
| Body | symmetric hinge limits | ±small delta |

### Mutation at Reproduction
- Small perturbation (±1–3% of range) on each parameter.
- No full-range replacement — gentle hill-climbing.
- All parameters inherited from parent with mutation.

### Success Criteria
- Critters develop visibly different gaits across generations.
- Average distance traveled per lifetime increases over generations.
- Population sustains itself (no extinction).

---

## Future Phases (sketch, not committed)

### Phase 3 — Brain controls speed + turn
- Minimal feedforward brain (few inputs → speed, turn outputs).
- Brain modulates CPG, does not replace it.
- Vision still disabled, eating still automatic.

### Phase 4 — Vision + food seeking
- Hardcoded 256→4 retina compression.
- Brain learns to steer toward food.

### Phase 5 — Eat motor
- Brain-controlled eating replaces automatic feeding.

### Phase 6 — CPG parameters via Lifetime Learning
- Brain or LTL system tunes CPG parameters within a lifetime.
- Reward: weighted combination of distance traveled and energy efficiency.
- Evolutionary CPG parameters become initial values that LTL refines.

### Phase 7+ — Species, predation, evolvable vision
- See `ROADMAP.md` for long-term vision.

---

## Integration with Existing Code

### Current files (Phase 1)
```
src/plugins/be_plugin_app_critterding/
    cpg_system.h / .cpp          — Central Pattern Generator
    critter_system.h / .cpp      — Brain/CPG switching, reproduction
config/body_plan.*.json          — CPG + body parameters
```

### Compatibility
The existing `Brainz` plugin remains unchanged.  CPG mode skips brain
creation entirely.  Both variants can coexist via config.

---

## Open Questions

1. **Which body parameters to evolve?** Start minimal (hinge limits, part
   dimensions) or include restitution, mass, etc.?
2. **Mutation rate tuning**: How much perturbation per generation?
   Needs experimentation.
3. **Population size**: Enough individuals for meaningful selection?
