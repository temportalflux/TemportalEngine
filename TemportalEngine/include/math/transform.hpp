#pragma once

#include "math/Matrix.hpp"


/*
template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> lookAtRH(vec<3, T, Q> const& eye, vec<3, T, Q> const& center, vec<3, T, Q> const& up)
	{
		vec<3, T, Q> const f(normalize(center - eye));
		vec<3, T, Q> const s(normalize(cross(f, up)));
		vec<3, T, Q> const u(cross(s, f));

		mat<4, 4, T, Q> Result(1);
		Result[0][0] = s.x;
		Result[1][0] = s.y;
		Result[2][0] = s.z;
		Result[0][1] = u.x;
		Result[1][1] = u.y;
		Result[2][1] = u.z;
		Result[0][2] =-f.x;
		Result[1][2] =-f.y;
		Result[2][2] =-f.z;
		Result[3][0] =-dot(s, eye);
		Result[3][1] =-dot(u, eye);
		Result[3][2] = dot(f, eye);
		return Result;
	}
*/

NS_MATH

/**
 * Returns a matrix which represents an object at `eyePos` looking at the position `center` where the `up` vector indicates the vertical direction.
 */
Matrix4x4 lookAt(Vector3 const &eyePos, Vector3 const &center, Vector3 const &up)
{
	auto f = (center - eyePos).normalized();
	auto s = Vector3::cross(f, up).normalized();
	auto u = Vector3::cross(s, f);
	auto result = Matrix4x4(1)
		.setRow(0, s)
		.setRow(1, u)
		.setRow(2, -f);
	result[3][0] = -s.dot(eyePos);
	result[3][1] = -u.dot(eyePos);
	result[3][2] = f.dot(eyePos);
	return result;
}

NS_END
