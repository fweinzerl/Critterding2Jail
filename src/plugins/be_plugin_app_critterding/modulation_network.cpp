#include "modulation_network.h"
#include "cpg_system.h"
#include "kernel/be_entity_core_types.h"
#include <cmath>

namespace
{
	constexpr float TWO_PI = 6.28318530717958647692f;

	float clamp(float v, float lo, float hi)
	{
		if ( v < lo ) return lo;
		if ( v > hi ) return hi;
		return v;
	}

	float wrap_phase(float p)
	{
		p = std::fmod(p, TWO_PI);
		if ( p < 0.0f ) p += TWO_PI;
		return p;
	}

	// Output index → CPG parameter mapping.
	// Keep in lock-step with mod_net_forward and mod_net_init_from_defaults.
	enum OutputIdx
	{
		OUT_FREQ = 0,
		OUT_BASE_SHOULDER,
		OUT_DELTA_SHOULDER,
		OUT_BASE_ELBOW,
		OUT_DELTA_ELBOW,
		OUT_BASE_DUTY,
		OUT_DELTA_DUTY,
		OUT_ELBOW_PHASE,
		OUT_SIDE_PHASE_OFFSET,
	};
}

void mod_net_init_from_defaults(ModulationNetwork& net, const CpgEvolvableParams& d)
{
	for ( int j = 0; j < ModulationNetwork::N_OUTPUTS; ++j )
		for ( int i = 0; i < ModulationNetwork::N_INPUTS; ++i )
			net.weights[j][i] = 0.0f;

	// Hardwired steering prior: scent gradient component in the body-right axis
	// drives the asymmetric shoulder/elbow amplitudes so the critter turns
	// toward food. Sign: delta > 0 strengthens the right side → rotates left,
	// so turning right (in_left_right > 0) needs a negative delta.
	/*net.weights[OUT_DELTA_SHOULDER][0] = -0.5f; // early "optimal" mod net
	net.weights[OUT_DELTA_ELBOW][0]    = -0.5f;*/

	net.bias[OUT_FREQ]              = d.frequency;
	net.bias[OUT_BASE_SHOULDER]     = d.base_shoulder_amplitude;
	net.bias[OUT_DELTA_SHOULDER]    = d.delta_shoulder_amplitude;
	net.bias[OUT_BASE_ELBOW]        = d.base_elbow_amplitude;
	net.bias[OUT_DELTA_ELBOW]       = d.delta_elbow_amplitude;
	net.bias[OUT_BASE_DUTY]         = d.base_duty_cycle;
	net.bias[OUT_DELTA_DUTY]        = d.delta_duty_cycle;
	net.bias[OUT_ELBOW_PHASE]       = d.elbow_phase;
	net.bias[OUT_SIDE_PHASE_OFFSET] = d.side_phase_offset;

	net.learning_rate = 0.0f; // start neutral; evolution discovers usefulness
	net.prev_field = 0.0f;
	net.prev_field_valid = false;
	net.ticks_since_apply = 0;
	net.pending_eat_apply = false;

	for ( int j = 0; j < ModulationNetwork::N_OUTPUTS; ++j )
		for ( int i = 0; i < ModulationNetwork::N_INPUTS; ++i )
			net.weight_accum[j][i] = 0.0f;

	for ( int i = 0; i < ModulationNetwork::N_INPUTS; ++i ) net.last_inputs[i] = 0.0f;
	for ( int j = 0; j < ModulationNetwork::N_OUTPUTS; ++j ) net.last_outputs[j] = 0.0f;
}

void mod_net_forward(ModulationNetwork& net, float in_left_right, float in_fwd_back, float in_field, CpgEvolvableParams& out)
{
	net.last_inputs[0] = in_left_right;
	net.last_inputs[1] = in_fwd_back;
	net.last_inputs[2] = in_field;

	float raw[ModulationNetwork::N_OUTPUTS];
	for ( int j = 0; j < ModulationNetwork::N_OUTPUTS; ++j )
	{
		raw[j] = net.bias[j]
		       + net.weights[j][0] * in_left_right
		       + net.weights[j][1] * in_fwd_back
		       + net.weights[j][2] * in_field;
		net.last_outputs[j] = raw[j];
	}

	out.frequency                = clamp(raw[OUT_FREQ],           0.002f, 0.5f);
	out.base_shoulder_amplitude  = clamp(raw[OUT_BASE_SHOULDER],  0.01f,  2.0f);
	out.delta_shoulder_amplitude = clamp(raw[OUT_DELTA_SHOULDER], -1.0f,  1.0f);
	out.base_elbow_amplitude     = clamp(raw[OUT_BASE_ELBOW],     0.01f,  2.0f);
	out.delta_elbow_amplitude    = clamp(raw[OUT_DELTA_ELBOW],    -1.0f,  1.0f);
	out.base_duty_cycle          = clamp(raw[OUT_BASE_DUTY],      0.1f,   0.9f);
	out.delta_duty_cycle         = clamp(raw[OUT_DELTA_DUTY],     -0.3f,  0.3f);
	out.elbow_phase              = wrap_phase(raw[OUT_ELBOW_PHASE]);
	out.side_phase_offset        = wrap_phase(raw[OUT_SIDE_PHASE_OFFSET]);
}

void mod_net_mutate(ModulationNetwork& net, BEntity* rng)
{
	if ( rng == 0 ) return;

	auto delta = [rng](float max_delta) -> float {
		rng->set( "min", Bint(-100) );
		rng->set( "max", Bint(100) );
		return max_delta * (0.01f * rng->get_int());
	};

	// Bias scales — same magnitudes as the old direct CPG mutations so initial
	// dynamics match what we had before the network was introduced.
	const float bias_scales[ModulationNetwork::N_OUTPUTS] = {
		0.01f, 0.05f, 0.03f, 0.05f, 0.03f, 0.02f, 0.01f, 0.15f, 0.15f
	};
	const float weight_scales[ModulationNetwork::N_OUTPUTS] = {
		0.005f, 0.025f, 0.015f, 0.025f, 0.015f, 0.010f, 0.005f, 0.05f, 0.05f
	};

	for ( int j = 0; j < ModulationNetwork::N_OUTPUTS; ++j )
	{
		net.bias[j] += delta(bias_scales[j]);
		for ( int i = 0; i < ModulationNetwork::N_INPUTS; ++i )
			net.weights[j][i] += delta(weight_scales[j]);
	}
	net.bias[OUT_ELBOW_PHASE]       = wrap_phase(net.bias[OUT_ELBOW_PHASE]);
	net.bias[OUT_SIDE_PHASE_OFFSET] = wrap_phase(net.bias[OUT_SIDE_PHASE_OFFSET]);

	// Learning rate: small additive walk, clamped to a sane range.
	net.learning_rate += delta(0.0005f);
	net.learning_rate = clamp(net.learning_rate, 0.0f, 0.05f);
}

void mod_net_hebbian_accumulate(ModulationNetwork& net, float h)
{
	if ( net.learning_rate <= 0.0f ) return;
	if ( h == 0.0f ) return;

	for ( int j = 0; j < ModulationNetwork::N_OUTPUTS; ++j )
	{
		const float out = net.last_outputs[j];
		net.weight_accum[j][0] += h * net.last_inputs[0] * out;
		net.weight_accum[j][1] += h * net.last_inputs[1] * out;
		net.weight_accum[j][2] += h * net.last_inputs[2] * out;
	}
}

void mod_net_hebbian_apply(ModulationNetwork& net)
{
	if ( net.learning_rate > 0.0f )
	{
		const float eta = net.learning_rate;
		for ( int j = 0; j < ModulationNetwork::N_OUTPUTS; ++j )
		{
			net.weights[j][0] += eta * net.weight_accum[j][0];
			net.weights[j][1] += eta * net.weight_accum[j][1];
			net.weights[j][2] += eta * net.weight_accum[j][2];
		}
	}
	for ( int j = 0; j < ModulationNetwork::N_OUTPUTS; ++j )
	{
		net.weight_accum[j][0] = 0.0f;
		net.weight_accum[j][1] = 0.0f;
		net.weight_accum[j][2] = 0.0f;
	}
}
