#include "scent_field.h"
#include <cmath>

ScentSample compute_scent(float x, float z, const std::vector<FoodPos>& food, float epsilon)
{
	ScentSample s = {0.0f, 0.0f, 0.0f};
	for (const auto& f : food)
	{
		float dx = x - f.x;
		float dz = z - f.z;
		float r = std::sqrt(dx * dx + dz * dz);
		float r_eps = r + epsilon;
		s.field += 1.0f / r_eps;
		if (r > 0.0001f)
		{
			float denom = r * r_eps * r_eps;
			s.grad_x += -dx / denom;
			s.grad_z += -dz / denom;
		}
	}
	return s;
}
