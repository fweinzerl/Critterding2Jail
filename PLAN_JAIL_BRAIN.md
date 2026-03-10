# JailBrain Plan

## Goal
Create a new plugin (`be_plugin_brain_jail`) for Critterding2Jail with a fixed modular brain architecture:
- `VisionRecognition`
- `ActionSelector`
- `MotionCenter`

The design must be explicit, constrained, and easier to learn than the current free recurrent graph.

## High-Level Architecture
Each critter brain is split into three feedforward modules with fixed wiring.
Every module uses **two hidden layers**.

Signal flow:
1. `VisionRecognition` -> feature vector
2. `ActionSelector` consumes feature vector + internal state
3. `MotionCenter` consumes selected action context + proprioception + oscillator, then drives motors

No arbitrary recurrent cross-connections between all nodes.

## Module 1: VisionRecognition
Purpose: compress retina into a small, meaningful feature vector.

Input:
- Retina values (initially can be `G` only, optional `RGBA` mode later)

Network:
- `Input -> Hidden1 -> Hidden2 -> OutputFeatures`

Output features (example initial set):
- `food_center_x`
- `food_center_y`
- `food_amount`
- `food_change`

Notes:
- Keep output feature size small and stable.
- This module is purely perceptual, no direct motor output.

## Module 2: ActionSelector
Purpose: choose high-level behavior from perception and internal context.

Input:
- Vision feature vector from `VisionRecognition`
- Internal state (at least `energy`, optionally `age`)

Network:
- `Input -> Hidden1 -> Hidden2 -> ActionWeights`

Initial actions:
- `seek_food`
- `eat`
- `idle`

Notes:
- Output should be normalized action weights (or a clear winner policy).
- Keep action set intentionally small in early iterations.

## Module 3: MotionCenter
Purpose: transform selected behavior into concrete motor commands.

Input:
- Action weights from `ActionSelector`
- Oscillator inputs (`osc_sin`, `osc_cos`)
- Proprioception (hinge angles)

Network:
- `Input -> Hidden1 -> Hidden2 -> MotorOutputs`

Output:
- Hinge motor targets (all controllable hinges)
- Optional: motor-level `eat` / `procreate` drive if required by current game loop

Notes:
- This is the only module that writes actuator values.
- Keep output bounded and deterministic.

## Constraints (Jail Rules)
- Fixed topology per module (no random structural growth/removal during runtime).
- Mutation/adaptation changes numeric parameters only (weights, biases, optional gains).
- Strict module boundaries and one-way data flow.
- Fail-fast on shape mismatch between body outputs and `MotionCenter` outputs.

## Integration Plan
1. Add new plugin `be_plugin_brain_jail`.
2. Add `JailBrain` entity with the 3-module pipeline.
3. Add critter-side factory switch to choose `Brainz` or `JailBrain`.
4. Keep existing `Brainz` plugin unchanged for comparison.
5. Add debug view values for:
   - Vision features
   - Action weights
   - Motor outputs

## Iteration 1 Scope (Minimal)
- Implement fixed forward pass only.
- Use simple random initialization in bounded ranges.
- No advanced training algorithm in this step.
- Verify end-to-end control works on the Mudskipper body.

## Lifetime Learning Integration
Use the existing episode-based lifetime loop and enforce strict mutation isolation:

- Each episode mutates **exactly one** module:
  - `VisionRecognition` or
  - `ActionSelector` or
  - `MotionCenter`
- The other two modules remain unchanged during that episode.
- After one episode, compare score:
  - better/equal: keep mutation
  - worse: rollback to pre-episode snapshot

Selection strategy for the module to mutate:
- Start with simple round-robin (`Vision -> Action -> Motion -> ...`) for clarity.
- Optionally switch later to weighted selection (for example more `MotionCenter` updates).

Rationale:
- Strong credit assignment: improvements can be attributed to one module.
- Fewer masked regressions from simultaneous multi-module changes.
- More stable progress under constrained JailBrain training.

## Future Extensions
- Add optional recurrent memory only inside selected modules.
- Add per-module learning rates.
- Add supervised pre-shaping for `VisionRecognition` features.
