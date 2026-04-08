#pragma once

#include <string>

class BEntity;
class CdCritter;
struct BodyPlanConfig;

// Evolvable CPG parameters — symmetric (left = mirror of right).
struct CpgEvolvableParams
{
	float frequency;
	float shoulder_amplitude;
	float elbow_amplitude;
	float elbow_phase;       // elbow offset relative to shoulder
	float side_phase_offset; // left side offset relative to right (pi = alternating)
};

// Structural layout — fixed per body plan, not evolvable.
struct CpgSymmetricLayout
{
	unsigned int right_shoulder;
	unsigned int right_elbow;
	unsigned int left_shoulder;
	unsigned int left_elbow;
	float shoulder_turn_gain;
	float elbow_turn_gain;
};

// Evolvable body dimensions — symmetric (left = right).
// Scales are half-extents per axis.
struct BodyEvolvableParams
{
	// torso
	float torso_scale_x;
	float torso_scale_y;
	float torso_scale_z;
	// shoulder joint part (right side, left mirrors)
	float shoulder_scale_x;
	float shoulder_scale_y;
	float shoulder_scale_z;
	// arm part (right side, left mirrors)
	float arm_scale_x;
	float arm_scale_y;
	float arm_scale_z;
};

// Central Pattern Generator — drives hinge constraints with rhythmic signals.
// Also manages symmetric body parameter evolution (shares the hinge layout).
// Optional per body plan: if the JSON has no cpg_frequency key, the CPG is inactive.
class CpgSystem
{
public:
	CpgSystem();

	// Load CPG config from a body plan JSON file.
	// Returns true if a CPG block was found and parsed successfully.
	bool loadConfig(const std::string& body_plan_path);

	bool enabled() const { return m_enabled; }
	const CpgEvolvableParams& defaultParams() const { return m_default_params; }
	const BodyEvolvableParams& defaultBodyParams() const { return m_default_body_params; }

	// Apply one tick of CPG output to a critter's constraints.
	void update(CdCritter* critter, float& cpg_phase, const CpgEvolvableParams& params, float speed, float turn);

	// Expand symmetric body params into a full BodyPlanConfig (modifying the default).
	void expandBodyParams(const BodyEvolvableParams& params, BodyPlanConfig& out) const;

	// Small perturbation on evolvable params. rng is the project's BEntity RNG.
	void mutate(CpgEvolvableParams& params, BEntity* rng) const;
	void mutateBody(BodyEvolvableParams& params, BEntity* rng) const;

private:
	bool m_enabled;
	CpgEvolvableParams m_default_params;
	BodyEvolvableParams m_default_body_params;
	CpgSymmetricLayout m_layout;
};
