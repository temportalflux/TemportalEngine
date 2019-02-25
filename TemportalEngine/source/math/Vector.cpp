#include "math/Vector.hpp"

NS_MATH

Quaternion const QuaternionFromAxisAngle(Vector3 const axis, float const angleRad)
{
	auto halfAngle = angleRad * 0.5f;
	return Quaternion(axis * std::sin(halfAngle))
		+ QuaternionIdentity * std::cos(halfAngle);
}

Quaternion const QuaternionConjugate(Quaternion const quat)
{
	// <-x, -y, -z, w>
	return Quaternion(-quat.subvector<3>())
		+ QuaternionIdentity * quat.w();
}

Quaternion const QuaternionInverse(Quaternion const quat)
{
	return QuaternionConjugate(quat).normalized();
}

Quaternion const QuaternionConcatenate(Quaternion const left, Quaternion const right)
{
	Vector3 lVec = left.subvector<3>();
	Vector3 rVec = right.subvector<3>();
	float w = left.w() * right.w() - Vector3::dot(lVec, rVec);
	Vector3 vec = lVec * w + rVec * w + Vector3::cross(lVec, rVec);
	return Quaternion(vec) + QuaternionIdentity * w;
}

Vector3 const RotateVector(Vector3 const vector, Quaternion const rotation)
{
	// vPrime = rot * v * rotConjugate
	// OR
	// r = quaternion vector
	// vPrime = v + 2r x (r x v + wv)
	return vector + Vector3::cross(
		rotation.subvector<3>() * 2,
		Vector3::cross(
			rotation.subvector<3>(), vector
		) * rotation.w() * vector
	);
}

Quaternion const MultiplyVector(Vector3 const vector, Quaternion const quat)
{
	// r = quat vector3
	// q* = < vxr + wv, -dot(v, r) >
	return Quaternion(
		Vector3::cross(vector, quat.subvector<3>())
		+ vector * quat.w()
	) + QuaternionIdentity * -Vector3::dot(vector, quat.subvector<3>());
}

Quaternion const IntegrateKinematic(Quaternion const rotation,
	Vector3 const angularVelocity, Vector3 const angularAcceleration, f32 const deltaTime)
{
	// qPrime = q + (dq/dt)dt + (1/2)(d^2q/dt^2)dt^2
	// qPrime = q + [(1/2)wdt * q] + [(1/2)aq - (1/4)|w|^2*q]dt^2
	Quaternion integrated = rotation;
	// integrate velocity
	integrated += MultiplyVector(0.5f * angularVelocity * deltaTime, rotation);
	// integrate acceleration
	integrated += (0.5f * deltaTime * deltaTime) * (
		MultiplyVector(0.5f * angularAcceleration, rotation)
		-
		(0.25f * angularVelocity.magnitudeSq() * rotation)
	);
	// normalize
	return integrated.normalized();
}

NS_END

using namespace math;

Vector2 const Vector2::zero = Vector2{ 0, 0 };
Vector2 const Vector2unitX = Vector2{ 1, 0 };
Vector2 const Vector2unitY = Vector2{ 0, 1 };
Vector3 const Vector3::zero = Vector3{ 0, 0, 0 };
Vector3 const Vector3unitX = Vector3{ 1, 0, 0 };
Vector3 const Vector3unitY = Vector3{ 0, 1, 0 };
Vector3 const Vector3unitZ = Vector3{ 0, 0, 1 };
Vector4 const Vector4::zero = Vector4{ 0, 0, 0, 0 };
Vector4 const Vector4unitX = Vector4{ 1, 0, 0, 0 };
Vector4 const Vector4unitY = Vector4{ 0, 1, 0, 0 };
Vector4 const Vector4unitZ = Vector4{ 0, 0, 1, 0 };
Vector4 const Vector4unitW = Vector4{ 0, 0, 0, 1 };
Quaternion const QuaternionIdentity = math::Vector4unitW;

Vector2Int const Vector2Int::zero = Vector2Int{ 0, 0 };
Vector3Int const Vector3Int::zero = Vector3Int{ 0, 0, 0 };

Vector2UInt const Vector2UInt::zero = Vector2UInt{ 0, 0 };
Vector3UInt const Vector3UInt::zero = Vector3UInt{ 0, 0, 0 };
