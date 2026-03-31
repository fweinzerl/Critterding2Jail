#include "cpg_system.h"
#include "critter_system.h"
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
}

CpgSystem::CpgSystem()
	: m_enabled(false)
{
}

bool CpgSystem::loadConfig(const std::string& body_plan_path)
{
	m_enabled = false;
	m_config.hinges.clear();

	std::string text;
	if ( !read_file_with_fallbacks(body_plan_path, text) )
	{
		return false;
	}

	// cpg_frequency is the sentinel — if absent, no CPG for this body plan
	if ( !json_float(text, "cpg_frequency", m_config.frequency) )
	{
		return false;
	}

	unsigned int hinge_count = 0;
	if ( !json_uint(text, "cpg_hinge_count", hinge_count) )
	{
		std::cerr << "ERROR: cpg_system: cpg_frequency present but cpg_hinge_count missing" << std::endl;
		std::exit(1);
	}

	m_config.hinges.reserve(hinge_count);
	for ( unsigned int i = 0; i < hinge_count; ++i )
	{
		CpgHingeConfig h;
		const auto id = std::to_string(i);
		bool ok = true;
		ok = ok && json_uint(text, "cpg_hinge_" + id + "_index", h.hinge_index);
		ok = ok && json_float(text, "cpg_hinge_" + id + "_phase", h.phase);
		ok = ok && json_float(text, "cpg_hinge_" + id + "_max_amplitude", h.max_amplitude);
		ok = ok && json_float(text, "cpg_hinge_" + id + "_turn_gain", h.turn_gain);
		if ( !ok )
		{
			std::cerr << "ERROR: cpg_system: incomplete cpg_hinge_" << id << " entry" << std::endl;
			std::exit(1);
		}
		m_config.hinges.push_back(h);
	}

	m_enabled = true;
	std::cout << "cpg_system: loaded " << hinge_count << " hinge configs, frequency=" << m_config.frequency << std::endl;
	return true;
}

void CpgSystem::update(CdCritter* critter, float& cpg_phase, float speed, float turn)
{
	if ( !m_enabled || critter == 0 ) return;

	auto constraints = critter->m_constraints_shortcut;
	if ( constraints == 0 ) return;

	// advance phase
	cpg_phase += m_config.frequency;
	if ( cpg_phase > TWO_PI )
	{
		cpg_phase = std::fmod(cpg_phase, TWO_PI);
	}

	const auto& constraint_children = constraints->children();

	for ( const auto& hcfg : m_config.hinges )
	{
		if ( hcfg.hinge_index >= constraint_children.size() ) continue;

		auto constraint_ref = constraint_children[hcfg.hinge_index];
		auto constraint = constraint_ref->get_reference();
		if ( constraint == 0 ) continue;

		float signal = speed * hcfg.max_amplitude * std::sin(cpg_phase + hcfg.phase)
		             + turn * hcfg.turn_gain;

		constraint->set(signal);
	}
}
