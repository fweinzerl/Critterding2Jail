#pragma once

#include <string>
#include <vector>

struct BodyPlanPart
{
	std::string name;
	float offset_x;
	float offset_y;
	float offset_z;
	float scale_x;
	float scale_y;
	float scale_z;
	bool has_friction_override;
	float friction_override;
};

struct BodyPlanHinge
{
	std::string name;
	unsigned int part_a;
	unsigned int part_b;
	float local_a_x;
	float local_a_y;
	float local_a_z;
	float local_a_pitch;
	float local_b_x;
	float local_b_y;
	float local_b_z;
	float local_b_pitch;
	float limit_low;
	float limit_high;
	float softness;
	float biasfactor;
	float relaxationfactor;
	bool bidirectional;
};

struct BodyPlanConfig
{
	std::string physics_bodypart_class;
	std::string graphics_model_name;
	std::string graphics_mesh_file;
	bool bodypart_use_density;
	float bodypart_density;
	float bodypart_friction;
	float bodypart_restitution;
	bool bodypart_wants_deactivation;
	std::vector<BodyPlanPart> parts;
	std::vector<BodyPlanHinge> hinges;
};

// Override config for body construction during reproduction.
// Set before triggering entity copy, clear after.
void cd_body_plan_set_override(const BodyPlanConfig* cfg);
const BodyPlanConfig& cd_body_plan_get_active();
