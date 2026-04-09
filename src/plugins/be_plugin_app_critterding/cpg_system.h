#pragma once

#include <string>

class BEntity;
class CdCritter;
struct BodyPlanConfig;

// Evolvable CPG parameters — base (symmetric) + delta (asymmetric).
// Right side uses (base + delta), left side uses (base - delta).
struct CpgEvolvableParams
{
	float frequency;
	float base_shoulder_amplitude;   // symmetric: shoulder strength
	float delta_shoulder_amplitude;  // asymmetric: shoulder left-right difference
	float base_elbow_amplitude;      // symmetric: elbow strength
	float delta_elbow_amplitude;     // asymmetric: elbow left-right difference
	float base_duty_cycle;           // symmetric: power-stroke ratio (0.5 = standard sinusoid)
	float delta_duty_cycle;          // asymmetric: left-right timing difference
	float elbow_phase;               // elbow offset relative to shoulder
	float side_phase_offset;         // left side offset relative to right (pi = alternating)
};

// Structural layout — fixed per body plan, not evolvable.
struct CpgSymmetricLayout
{
	unsigned int right_shoulder;
	unsigned int right_elbow;
	unsigned int left_shoulder;
	unsigned int left_elbow;
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
	void update(CdCritter* critter, float& cpg_phase, const CpgEvolvableParams& params);

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
