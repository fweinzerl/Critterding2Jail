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

---

## Module 1: Central Pattern Generator (CPG)

### What it does
A hard-wired oscillator network that drives the critter's hinges in a rhythmic
walking pattern.  The brain does NOT generate the rhythm — it only modulates it.

### Interface to brain
The CPG exposes two control parameters:

| Parameter | Range | Effect |
|-----------|-------|--------|
| `speed`   | 0.0 – 1.0 | Amplitude of the oscillation (0 = stand still, 1 = full stride) |
| `turn`    | -1.0 – 1.0 | Asymmetry between left/right legs (0 = straight, ±1 = sharp turn) |

### Implementation sketch
```
for each hinge h in body:
    phase_h    = base_phase[h]                // fixed per body plan
    amplitude  = speed * max_amplitude[h]     // modulated by brain
    offset     = turn * turn_gain[h]          // positive for left legs, negative for right
    target[h]  = amplitude * sin(tick * frequency + phase_h) + offset
```

### Per body plan
Each body plan JSON (`body_plan.default.json`, `body_plan.mudskipper.json`) gets a
CPG parameter block:

```json
"cpg": {
    "frequency": 0.05,
    "hinges": [
        { "name": "hinge_t1_t2",   "phase": 0.0,   "max_amplitude": 0.4, "turn_gain": 0.0  },
        { "name": "hinge_t2_t3",   "phase": 3.14,  "max_amplitude": 0.4, "turn_gain": 0.0  },
        { "name": "hinge_t1_j_l",  "phase": 0.0,   "max_amplitude": 0.3, "turn_gain": 0.15 },
        { "name": "hinge_t1_j_r",  "phase": 0.0,   "max_amplitude": 0.3, "turn_gain":-0.15 }
    ]
}
```

Phase offsets and turn gains need to be tuned once per body, then stay fixed.
This is the equivalent of "spinal cord wiring" — it does not evolve.

---

## Module 2: Hardcoded Vision Compression

### What it does
Reduces the 8×8×4 (256) retina values to 4 meaningful signals before they reach
the brain.

### Output signals

| Signal | Computation | Meaning |
|--------|-------------|---------|
| `food_dir_x` | Weighted horizontal centre-of-mass of green pixels | Where is food, left/right? |
| `food_dir_y` | Weighted vertical centre-of-mass of green pixels | Where is food, up/down? |
| `food_proximity` | Sum of green channel values, normalized | How much/close is food? |
| `obstacle_ahead` | Average brightness of center 2×2 pixels | Something blocking the way? |

### Implementation
Runs every frame in `vision_system.cpp` after the pixel readback, before brain
input assignment.  Replaces the 256 `vision_value_*` brain inputs with 4
`vision_compressed_*` inputs.

### Why hardcoded?
Evolution cannot discover spatial convolution through random synapse mutations.
A weight from pixel (3,2,G) to neuron N has no neighbourhood relationship to the
weight from pixel (3,3,G) to neuron N.  Small mutations cannot gradually build a
"look towards green" detector — they just add noise.

Later (Phase 5+) the compression can become evolvable, but only after the
brain already uses the compressed signals effectively.

---

## Module 3: Minimal Brain (speed + turn + eat)

### Architecture
Fixed-topology feedforward network, no spiking, no threshold:

```
Inputs (6):
    vision/food_dir_x
    vision/food_dir_y
    vision/food_proximity
    vision/obstacle_ahead
    internal/energy        (normalized 0–1)
    internal/age           (normalized 0–1)
        │
   Hidden Layer (8 neurons, tanh activation)
        │
   Outputs (3):
    motor/speed            (sigmoid → 0–1)
    motor/turn             (tanh → -1 – 1)
    motor/eat              (sigmoid → 0–1, only used from Phase 4)
```

### Why not spiking?
Spiking networks need temporal dynamics to transmit information.  With 6 inputs
and 3 outputs, a simple feedforward net is sufficient and much easier to evolve.
Spiking can be reintroduced later for richer temporal behaviours.

### Weight count
6×8 + 8 bias + 8×3 + 3 bias = 48 + 8 + 24 + 3 = **83 parameters**.
This is small enough that gentle mutation can explore it in reasonable time.

---

## Module 4: Gentler Mutation

### Changes to mutation parameters

| Mutation type | Current weight | New weight | Change |
|---------------|---------------|------------|--------|
| Add neuron | 25 | 0 | Fixed topology — not applicable |
| Remove neuron | 25 | 0 | Fixed topology — not applicable |
| Alter weight (full replace) | 80 | 5 | Near-elimination |
| Alter weight (slight) | 160 | 200 | Primary mutation mode |
| Slight perturbation range | ±5% of full range | ±1% of full range | Finer steps |

### Rationale
With only 83 parameters, we need stable inheritance.  A single full-range
replacement can destroy a working food-seeking circuit.  1% perturbations allow
gradual hill-climbing on the fitness landscape.

---

## Phased Bootstrapping

### Phase 1 — Validate CPG
- CPG drives all hinges, brain disconnected
- Essen automatisch bei Kontakt (kein `eat`-Motor nötig)
- **Ziel**: Kritter laufen sichtbar, Physik stabil
- **Erfolgskriterium**: Kritter bewegen sich durch die Welt, kein Absturz

### Phase 2 — Brain steuert speed + turn
- Brain-Outputs `speed` und `turn` modulieren CPG
- Vision noch deaktiviert (konstante Dummy-Werte)
- Essen weiterhin automatisch
- **Ziel**: Evolution entdeckt, dass Bewegung die Chance auf Futter und Reproduktion erhoeht
- **Erfolgskriterium**: Nach N Generationen bewegen sich Kritter mehr als zufällig

### Phase 3 — Vision aktiv
- 4 komprimierte Vision-Inputs werden ans Gehirn angeschlossen
- **Ziel**: Kritter orientieren sich in Richtung Futter
- **Erfolgskriterium**: Durchschnittliche Distanz zum nächsten Futter sinkt über Generationen

### Phase 4 — Eat-Motor reaktivieren
- `eat`-Output wird vom Gehirn gesteuert
- Essen nur bei Kontakt + `eat > 0.5`
- **Ziel**: Kritter lernen, bei Futterkontakt zu essen
- **Erfolgskriterium**: Eat-Output korreliert mit Futterkollisionen

### Phase 5 — Vision evolvierbar machen
- Hardcoded Kompression durch ein kleines evolvierbares Netzwerk ersetzen
- Input: 256 Retina-Werte → Hidden (16 Neuronen) → 4 Features
- **Ziel**: Feinere Wahrnehmung als die hardcoded Version
- **Erfolgskriterium**: Fitness steigt gegenüber Phase 4

---

## Integration mit bestehendem Code

### Neue Dateien
```
src/plugins/be_plugin_app_critterding/
    cpg_system.h / .cpp          — Central Pattern Generator
    vision_compression.h / .cpp  — Hardcoded 256→4 compression
    iron_brain.h / .cpp          — Minimal feedforward brain
```

### Geänderte Dateien
```
critter_system.cpp   — Brain type switch, CPG integration
vision_system.cpp    — Compression step after pixel readback
plugin.cpp           — Phase flags, optional eat-auto mode
body_system.cpp      — CPG parameter loading from body plan JSON
config/body_plan.*.json — CPG parameter blocks
```

### Kompatibilität
Das bestehende `Brainz`-Plugin bleibt unverändert.  Ein Settings-Flag
`brain_type` (default: `"brainz"`) wählt zwischen dem klassischen System und
Iron Maiden.  So können beide Varianten direkt verglichen werden.

---

## Lifetime Learning

### Aktueller Stand (ai_codex Branch)
- Leichtgewichtige Lifetime-Adaptation durch episodenbasierte Parametermutation.
- Oszillator-Kontext pro Kritter: `oscillator/frequency`, `oscillator/phase`,
  Brain-Inputs `osc_sin` und `osc_cos`.
- Frequenz wird bei Reproduktion mit leichter Mutation vererbt.
- Episode-Laenge: `learning_episode_ticks = 300`.

### Integration mit Iron Maiden
Die episodenbasierte Mutation aus dem bestehenden Code passt gut zum Iron Maiden
Ansatz, muss aber angepasst werden:

- **Pro Episode wird genau ein Modul mutiert** (Vision ODER Brain ODER CPG-Parameter
  falls spaeter evolvierbar).  Die anderen bleiben eingefroren.
- Nach einer Episode: Score vergleichen.
  - Besser/gleich → Mutation behalten.
  - Schlechter → Rollback auf Pre-Episode-Snapshot.
- Start mit Round-Robin (`Vision → Brain → Vision → ...`), spaeter optional
  gewichtete Selektion.

### Vorteile gegenueber dem aktuellen System
- **Credit Assignment**: Verbesserungen koennen einem Modul zugeordnet werden.
- **Weniger maskierte Regressionen** durch gleichzeitige Multi-Modul-Aenderungen.
- **Stabilerer Fortschritt** bei nur 83 Parametern.

### Naechste Schritte fuer Observability
- Avg Distanzgewinn pro Zeiteinheit.
- Episode-Reward-Trend.
- Oszillator-Frequenzverteilung in der Population.

### Nicht-Ziele (vorerst)
- Kein NEAT / Topologie-Evolution.
- Kein hierarchischer Policy-Stack.
- Kein Raeuber-spezifisches Reward-Shaping (kommt erst mit Roadmap Iteration 1).

---

## Offene Fragen

1. **CPG-Tuning**: Die Phase-Offsets und Turn-Gains müssen manuell pro Körperplan
   eingestellt werden.  Erster Ansatz: Trial-and-error im Simulator mit fixem
   `speed=1.0`, `turn=0.0`.
2. **Futter-Farbe**: Ist Futter immer grün gerendert?  Die Vision-Kompression
   hängt davon ab.  Falls nicht, muss ein anderes Merkmal verwendet werden
   (z.B. Helligkeit/Größe).
