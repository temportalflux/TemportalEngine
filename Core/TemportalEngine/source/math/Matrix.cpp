#include "math/Matrix.hpp"

math::Matrix4x4 math::createModelMatrix(Vector3 const pos)
{
	auto m = math::Matrix4x4(1);
	m[3] = (m[0] * pos[0]) + (m[1] * pos[1]) + (m[2] * pos[2]) + m[3];
	return m;
}
