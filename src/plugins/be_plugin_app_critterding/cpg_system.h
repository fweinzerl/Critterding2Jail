#pragma once

#include <string>
#include <vector>

class BEntity;
class CdCritter;

struct CpgHingeConfig
{
	unsigned int hinge_index;
	float phase;
	float max_amplitude;
	float turn_gain;
};

struct CpgConfig
{
	float frequency;
	std::vector<CpgHingeConfig> hinges;
};

// Central Pattern Generator — drives hinge constraints with rhythmic signals.
// Optional per body plan: if the JSON has no cpg_frequency key, the CPG is inactive.
class CpgSystem
{
public:
	CpgSystem();

	// Load CPG config from a body plan JSON file.
	// Returns true if a CPG block was found and parsed successfully.
	bool loadConfig(const std::string& body_plan_path);

	bool enabled() const { return m_enabled; }
	const CpgConfig& config() const { return m_config; }

	// Apply one tick of CPG output to a critter's constraints.
	// cpg_phase is the critter's accumulated phase (updated in place).
	// speed: 0..1, turn: -1..1 (Phase 1: fixed 1.0 and 0.0).
	void update(CdCritter* critter, float& cpg_phase, float speed, float turn);

private:
	bool m_enabled;
	CpgConfig m_config;
};
