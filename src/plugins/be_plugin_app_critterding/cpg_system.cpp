#include "cpg_system.h"
#include "critter_system.h"
#include "body_plan_config.h"
#include "kernel/be_entity_core_types.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include <iostream>

namespace
{
	constexpr float TWO_PI = 6.28318530717958647692f;

	bool read_file(const std::string& path, std::string& out)
	{
		std::ifstream f(path.c_str(), std::ios::in);
		if ( !f.is_open() ) return false;
		out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
		return true;
	}

	bool read_file_with_fallbacks(const std::string& path, std::string& out)
	{
		if ( read_file(path, out) ) return true;
		if ( read_file("../" + path, out) ) return true;
		if ( read_file("../../" + path, out) ) return true;
		return false;
	}

	bool json_find_value(const std::string& text, const std::string& key, std::string& out)
	{
		std::string pattern("\"");
		pattern += key;
		pattern += "\"";
		const auto pos = text.find(pattern);
		if ( pos == std::string::npos ) return false;

		auto colon = text.find(':', pos + pattern.size());
		if ( colon == std::string::npos ) return false;

		auto start = colon + 1;
		while ( start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) )
			++start;
		if ( start >= text.size() ) return false;

		auto end = start;
		while ( end < text.size() )
		{
			const auto c = text[end];
			if ( c == ',' || c == '}' || c == '\n' || c == '\r' ) break;
			++end;
		}
		out = text.substr(start, end - start);
		return true;
	}

	bool json_float(const std::string& text, const std::string& key, float& target)
	{
		std::string raw;
		if ( !json_find_value(text, key, raw) ) return false;
		std::istringstream s(raw);
		s.imbue(std::locale::classic());
		s >> target;
		return !s.fail();
	}

	bool json_uint(const std::string& text, const std::string& key, unsigned int& target)
	{
		float v(0.0f);
		if ( !json_float(text, key, v) ) return false;
		if ( v < 0.0f ) return false;
		target = static_cast<unsigned int>(v);
		return true;
	}

	float wrap_phase(float p)
	{
		p = std::fmod(p, TWO_PI);
		if ( p < 0.0f ) p += TWO_PI;
		return p;
	}
}

CpgSystem::CpgSystem()
	: m_enabled(false)
	, m_default_params{0.0f, 0.0f, 0.0f, 0.0f, 0.0f}
	, m_default_body_params{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}
	, m_layout{0, 0, 0, 0, 0.0f, 0.0f}
{
}

bool CpgSystem::loadConfig(const std::string& body_plan_path)
{
	m_enabled = false;

	std::string text;
	if ( !read_file_with_fallbacks(body_plan_path, text) )
	{
		return false;
	}

	// cpg_frequency is the sentinel — if absent, no CPG for this body plan
	if ( !json_float(text, "cpg_frequency", m_default_params.frequency) )
	{
		return false;
	}

	// evolvable params
	bool ok = true;
	ok = ok && json_float(text, "cpg_shoulder_amplitude", m_default_params.shoulder_amplitude);
	ok = ok && json_float(text, "cpg_elbow_amplitude", m_default_params.elbow_amplitude);
	ok = ok && json_float(text, "cpg_elbow_phase", m_default_params.elbow_phase);
	ok = ok && json_float(text, "cpg_side_phase_offset", m_default_params.side_phase_offset);
	if ( !ok )
	{
		std::cerr << "ERROR: cpg_system: cpg_frequency present but symmetric params incomplete" << std::endl;
		std::exit(1);
	}

	// structural layout
	ok = true;
	ok = ok && json_uint(text, "cpg_hinge_right_shoulder", m_layout.right_shoulder);
	ok = ok && json_uint(text, "cpg_hinge_right_elbow", m_layout.right_elbow);
	ok = ok && json_uint(text, "cpg_hinge_left_shoulder", m_layout.left_shoulder);
	ok = ok && json_uint(text, "cpg_hinge_left_elbow", m_layout.left_elbow);
	ok = ok && json_float(text, "cpg_shoulder_turn_gain", m_layout.shoulder_turn_gain);
	ok = ok && json_float(text, "cpg_elbow_turn_gain", m_layout.elbow_turn_gain);
	if ( !ok )
	{
		std::cerr << "ERROR: cpg_system: symmetric layout keys incomplete" << std::endl;
		std::exit(1);
	}

	// body evolvable params: read part scales (torso=part_0, shoulder=part_1, arm=part_3)
	ok = true;
	ok = ok && json_float(text, "part_0_scale_x", m_default_body_params.torso_scale_x);
	ok = ok && json_float(text, "part_0_scale_y", m_default_body_params.torso_scale_y);
	ok = ok && json_float(text, "part_0_scale_z", m_default_body_params.torso_scale_z);
	ok = ok && json_float(text, "part_1_scale_x", m_default_body_params.shoulder_scale_x);
	ok = ok && json_float(text, "part_1_scale_y", m_default_body_params.shoulder_scale_y);
	ok = ok && json_float(text, "part_1_scale_z", m_default_body_params.shoulder_scale_z);
	ok = ok && json_float(text, "part_3_scale_x", m_default_body_params.arm_scale_x);
	ok = ok && json_float(text, "part_3_scale_y", m_default_body_params.arm_scale_y);
	ok = ok && json_float(text, "part_3_scale_z", m_default_body_params.arm_scale_z);
	if ( !ok )
	{
		std::cerr << "ERROR: cpg_system: part scale keys incomplete for body evolution" << std::endl;
		std::exit(1);
	}

	m_enabled = true;
	std::cout << "cpg_system: loaded symmetric config, frequency=" << m_default_params.frequency
	          << " shoulder_amp=" << m_default_params.shoulder_amplitude
	          << " elbow_amp=" << m_default_params.elbow_amplitude << std::endl;
	return true;
}

void CpgSystem::update(CdCritter* critter, float& cpg_phase, const CpgEvolvableParams& params, float speed, float turn)
{
	if ( !m_enabled || critter == 0 ) return;

	auto constraints = critter->m_constraints_shortcut;
	if ( constraints == 0 ) return;

	cpg_phase += params.frequency;
	if ( cpg_phase > TWO_PI )
	{
		cpg_phase = std::fmod(cpg_phase, TWO_PI);
	}

	const auto& cc = constraints->children();

	// right shoulder
	if ( m_layout.right_shoulder < cc.size() )
	{
		auto c = cc[m_layout.right_shoulder]->get_reference();
		if ( c ) c->set( speed * params.shoulder_amplitude * std::sin(cpg_phase)
		               + turn * m_layout.shoulder_turn_gain );
	}

	// right elbow
	if ( m_layout.right_elbow < cc.size() )
	{
		auto c = cc[m_layout.right_elbow]->get_reference();
		if ( c ) c->set( speed * params.elbow_amplitude * std::sin(cpg_phase + params.elbow_phase)
		               + turn * m_layout.elbow_turn_gain );
	}

	// left shoulder (mirrored body: negate signal so both arms swing same world direction)
	if ( m_layout.left_shoulder < cc.size() )
	{
		auto c = cc[m_layout.left_shoulder]->get_reference();
		if ( c ) c->set( -speed * params.shoulder_amplitude * std::sin(cpg_phase + params.side_phase_offset)
		                - turn * m_layout.shoulder_turn_gain );
	}

	// left elbow (mirrored: phase offset, negated turn gain)
	if ( m_layout.left_elbow < cc.size() )
	{
		auto c = cc[m_layout.left_elbow]->get_reference();
		if ( c ) c->set( speed * params.elbow_amplitude * std::sin(cpg_phase + params.side_phase_offset + params.elbow_phase)
		               - turn * m_layout.elbow_turn_gain );
	}
}

void CpgSystem::mutate(CpgEvolvableParams& params, BEntity* rng) const
{
	if ( rng == 0 ) return;

	// small random delta: rng returns int in [min,max], scale to float
	auto delta = [rng](float max_delta) -> float {
		rng->set( "min", Bint(-100) );
		rng->set( "max", Bint(100) );
		return max_delta * (0.01f * rng->get_int());
	};

	params.frequency += delta(0.01f);
	if ( params.frequency < 0.002f ) params.frequency = 0.002f;
	if ( params.frequency > 0.5f ) params.frequency = 0.5f;

	params.shoulder_amplitude += delta(0.05f);
	if ( params.shoulder_amplitude < 0.01f ) params.shoulder_amplitude = 0.01f;
	if ( params.shoulder_amplitude > 2.0f ) params.shoulder_amplitude = 2.0f;

	params.elbow_amplitude += delta(0.05f);
	if ( params.elbow_amplitude < 0.01f ) params.elbow_amplitude = 0.01f;
	if ( params.elbow_amplitude > 2.0f ) params.elbow_amplitude = 2.0f;

	params.elbow_phase = wrap_phase(params.elbow_phase + delta(0.15f));
	params.side_phase_offset = wrap_phase(params.side_phase_offset + delta(0.15f));
}

void CpgSystem::expandBodyParams(const BodyEvolvableParams& params, BodyPlanConfig& cfg) const
{
	cfg = cd_body_plan_get_active();
	if ( cfg.parts.size() < 5 || cfg.hinges.size() < 4 ) return;

	// ratio-based scaling: preserve original body plan proportions
	const float r_torso_x    = params.torso_scale_x    / m_default_body_params.torso_scale_x;
	const float r_shoulder_x = params.shoulder_scale_x / m_default_body_params.shoulder_scale_x;
	const float r_arm_x      = params.arm_scale_x      / m_default_body_params.arm_scale_x;

	// torso (center, no mirror)
	cfg.parts[0].scale_x = params.torso_scale_x;
	cfg.parts[0].scale_y = params.torso_scale_y;
	cfg.parts[0].scale_z = params.torso_scale_z;

	// shoulder offset scales with torso width; arm delta scales with shoulder width
	const float default_shoulder_offset = cfg.parts[1].offset_x;
	const float default_arm_delta = cfg.parts[3].offset_x - cfg.parts[1].offset_x;
	const float new_shoulder_offset = default_shoulder_offset * r_torso_x;
	const float new_arm_offset = new_shoulder_offset + default_arm_delta * r_shoulder_x;

	// shoulder pair (right=1, left=2)
	const unsigned int pairs[][2] = { {1, 2}, {3, 4} };
	const float new_scales[][3] = {
		{ params.shoulder_scale_x, params.shoulder_scale_y, params.shoulder_scale_z },
		{ params.arm_scale_x,      params.arm_scale_y,      params.arm_scale_z },
	};
	const float offsets[] = { new_shoulder_offset, new_arm_offset };

	for ( int p = 0; p < 2; ++p )
	{
		for ( int side = 0; side < 2; ++side )
		{
			unsigned int idx = pairs[p][side];
			float sign = (side == 0) ? 1.0f : -1.0f;
			cfg.parts[idx].scale_x = new_scales[p][0];
			cfg.parts[idx].scale_y = new_scales[p][1];
			cfg.parts[idx].scale_z = new_scales[p][2];
			cfg.parts[idx].offset_x = sign * offsets[p];
		}
	}

	// hinge attachments scale with their host part's x-dimension
	// hinges 0,2: torso(A) → shoulder(B)
	cfg.hinges[0].local_a_x *= r_torso_x;
	cfg.hinges[0].local_b_x *= r_shoulder_x;
	cfg.hinges[2].local_a_x *= r_torso_x;
	cfg.hinges[2].local_b_x *= r_shoulder_x;

	// hinges 1,3: shoulder(A) → arm(B)
	cfg.hinges[1].local_a_x *= r_shoulder_x;
	cfg.hinges[1].local_b_x *= r_arm_x;
	cfg.hinges[3].local_a_x *= r_shoulder_x;
	cfg.hinges[3].local_b_x *= r_arm_x;
}

void CpgSystem::mutateBody(BodyEvolvableParams& params, BEntity* rng) const
{
	if ( rng == 0 ) return;

	auto delta = [rng](float max_delta) -> float {
		rng->set( "min", Bint(-100) );
		rng->set( "max", Bint(100) );
		return max_delta * (0.01f * rng->get_int());
	};

	auto clamp = [](float& v, float lo, float hi) {
		if ( v < lo ) v = lo;
		if ( v > hi ) v = hi;
	};

	// additive mutation: ±3% of parameter range (unbiased)
	float* vals[] = {
		&params.torso_scale_x,    &params.torso_scale_y,    &params.torso_scale_z,
		&params.shoulder_scale_x, &params.shoulder_scale_y, &params.shoulder_scale_z,
		&params.arm_scale_x,      &params.arm_scale_y,      &params.arm_scale_z,
	};
	const float lo[] = { 0.1f, 0.1f, 0.1f,  0.05f, 0.05f, 0.05f,  0.05f, 0.05f, 0.05f };
	const float hi[] = { 2.0f, 2.0f, 2.0f,  1.0f,  1.0f,  1.0f,   1.5f,  1.0f,  1.0f  };

	for ( int i = 0; i < 9; ++i )
	{
		*vals[i] += delta((hi[i] - lo[i]) * 0.03f);
		clamp(*vals[i], lo[i], hi[i]);
	}
}
