#pragma once

class BEntity;
struct CpgEvolvableParams;

// Modulation network: linear mapping from scent inputs to CPG parameters.
// Inputs (all in the critter's body frame, gradient normalized):
//   in_left_right — right (+) / left (-) component of the scent gradient direction
//   in_fwd_back   — forward/back component of the scent gradient direction
//   in_field      — raw scent field magnitude
// Outputs: frequency, base/delta shoulder amp, base/delta elbow amp,
// base/delta duty cycle, elbow_phase, side_phase_offset.
//
// The bias values play the role of the previous "default CPG params" — the
// network at zero input must already produce a functional gait. Lifetime
// learning is Hebbian, gated by a hormone signal derived from the scent field.
struct ModulationNetwork
{
	static constexpr int N_INPUTS = 3;
	static constexpr int N_OUTPUTS = 9;

	float weights[N_OUTPUTS][N_INPUTS] {};
	float bias[N_OUTPUTS] {};

	// Lifetime learning state
	float learning_rate = 0.0f;   // global eta, evolvable
	float prev_field = 0.0f;
	bool  prev_field_valid = false;
	unsigned int ticks_since_apply = 0;
	// plugin.cpp sets this on eat; critter_system applies + resets next tick.
	bool pending_eat_apply = false;

	// Per-tick Hebbian product accumulator.
	//   accum[j][i] += delta_field_t · last_inputs[i] · last_outputs[j]
	// Accumulating the *product* (not the hormone scalar) preserves the
	// per-tick correlation between reward and gait phase, and the window can
	// be long without teleskop confounders from eat or other critters.
	float weight_accum[N_OUTPUTS][N_INPUTS] {};

	// Forward-pass cache, used by the tick's accumulation step.
	float last_inputs[N_INPUTS] {};
	float last_outputs[N_OUTPUTS] {};
};

// Initialize a fresh network from CPG defaults: biases match defaults, weights = 0.
void mod_net_init_from_defaults(ModulationNetwork& net, const CpgEvolvableParams& defaults);

// Forward pass: read scent inputs, write clamped CPG parameters.
void mod_net_forward(ModulationNetwork& net, float in_left_right, float in_fwd_back, float in_field, CpgEvolvableParams& out);

// Mutate weights, biases, side params and learning rate.
void mod_net_mutate(ModulationNetwork& net, BEntity* rng);

// Accumulate one tick's Hebbian contribution: accum[j][i] += h · I[i] · O[j].
// h is the tick's hormone (usually the field delta; also used by plugin.cpp
// to deposit the eat reward with the pre-eat I/O snapshot still in last_*).
// No-op if learning_rate is zero.
void mod_net_hebbian_accumulate(ModulationNetwork& net, float h);

// Apply accumulator to weights (w += η · accum) and reset accum.
void mod_net_hebbian_apply(ModulationNetwork& net);
