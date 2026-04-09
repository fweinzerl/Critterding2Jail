#pragma once

#include <vector>

struct ScentSample
{
	float field;
	float grad_x;
	float grad_z;
};

struct FoodPos
{
	float x;
	float z;
};

// compute scent field value and gradient at (x, z) from food positions
ScentSample compute_scent(float x, float z, const std::vector<FoodPos>& food, float epsilon);
