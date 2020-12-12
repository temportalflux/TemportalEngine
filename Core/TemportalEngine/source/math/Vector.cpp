#include "math/Vector.hpp"

NS_MATH

Quaternion Quaternion::FromAxisAngle(Vector<f32, 3> axis, f32 radians)
{
	auto halfAngle = radians * 0.5f;
	return Quaternion(axis * std::sin(halfAngle))
		+ Quaternion::Identity * std::cos(halfAngle);
}

// See https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Quaternion_to_Euler_Angles_Conversion
Vector<f32, 3> Quaternion::euler() const
{
	Vector3 euler;

	euler.x(std::atan2(
		2 * (this->w() * this->x() + this->y() * this->z()),
		1 - 2 * (this->x() * this->x() + this->z() * this->z())
	));

	auto sinp = 2 * (this->w() * this->z() - this->y() * this->x());
	euler.z(
		// use 90 degrees if out of range
		std::abs(sinp) >= 1 ? std::copysign((f32)math::pi2(), sinp) : std::asin(sinp)
	);

	euler.y(std::atan2(
		2 * (this->w() * this->y() + this->x() * this->z()),
		1 - 2 * (this->y() * this->y() + this->z() * this->z())
	));

	return euler;
}

Quaternion Quaternion::conjugate() const
{
	// <-x, -y, -z, w>
	return Quaternion(-createSubvector<3>()) + Quaternion::Identity * w();
}

Quaternion Quaternion::inverseQuat() const
{
	return this->conjugate().normalized();
}

/* glm::rotate
	this->w = p.w * q.w - p.x * q.x - p.y * q.y - p.z * q.z;
	this->x = p.w * q.x + p.x * q.w + p.y * q.z - p.z * q.y;
	this->y = p.w * q.y + p.y * q.w + p.z * q.x - p.x * q.z;
	this->z = p.w * q.z + p.z * q.w + p.x * q.y - p.y * q.x;
*/
Quaternion Quaternion::concat(Quaternion const &a, Quaternion const &b)
{
	/* https://en.wikipedia.org/wiki/Quaternion#Hamilton_product
		a1a2 - b1b2 - c1c2 - d1d2
		+ (a1b2 + b1a2 + c1d2 - d1c2)i
		+ (a1c2 - b1d2 + c1a2 + d1b2)j
		+ (a1d2 + b1c2 - c1b2 + d1a2)k
	*/
	/* https://en.wikipedia.org/wiki/Quaternion#Hamilton_product
		where:
			a1+b1i+c1j+d1k = `a<x, y, z, w>`
			a2+b2i+c2j+d2k = `b<x, y, z, w>`
		w: awbw - axbx - ayby - azbz
		x: awbx + axbw + aybz - azby
		y: awby - axbz + aybw + azbx
		z: awbz + axby - aybx + azbw
	*/
	Vector3 aRealVec = a.createSubvector<3>();
	Vector3 bRealVec = b.createSubvector<3>();
	// aw*bw - (ax*bx + ay*by + az*bz)
	float w = a.w() * b.w() - Vector3::dot(aRealVec, bRealVec);
	// <ay*bz - az*by, az*bx - ax*bz, ax*by - ay*bx>
	auto crossReal = Vector3::cross(aRealVec, bRealVec);
	/*
		v: aw*b + bw*a + cross
		x: awbx + axbw + (aybz - azby)
		y: awby + aybw + (azbx - axbz)
		z: awbz + azbw + (axby - aybx)
	*/
	Vector3 vec = (a.w() * bRealVec) + (b.w() * aRealVec) + crossReal;
	return Quaternion(vec) + Quaternion::Identity * w;
}

Vector<f32, 3> Quaternion::rotate(Vector<f32, 3> const v) const
{
	auto qVec = createSubvector<3>();
	auto a = 2 * qVec.dot(v) * qVec;
	auto b = (w() * w() - qVec.magnitudeSq()) * v;
	auto c = 2 * w() * Vector3::cross(qVec, v);
	return a + b + c;
}

Vector2 const Vector2::ZERO = { 0, 0 };
Vector2Int const Vector2Int::ZERO = { 0, 0 };
Vector2UInt const Vector2UInt::ZERO = { 0, 0 };
Vector2 const Vector2unitX = { 1, 0 };
Vector2 const Vector2unitY = { 0, 1 };
Vector2 const V2_RIGHT = Vector2unitX;
Vector2 const V2_UP = Vector2unitY;

Vector3 const Vector3::ZERO = { 0, 0, 0 };
Vector3Int const Vector3Int::ZERO = { 0, 0, 0 };
Vector3UInt const Vector3UInt::ZERO = { 0, 0, 0 };
Vector3 const Vector3unitX = { 1, 0, 0 };
Vector3 const Vector3unitY = { 0, 1, 0 };
Vector3 const Vector3unitZ = { 0, 0, 1 };
// Y-Up Right-Handed is +X, +Y, -Z
Vector3 const V3_RIGHT = Vector2unitX;
Vector3 const V3_UP = Vector2unitY;
Vector3 const V3_FORWARD = -Vector3unitZ;

Vector4 const Vector4::ZERO = { 0, 0, 0, 0 };
Vector4 const Vector4unitX = { 1, 0, 0, 0 };
Vector4 const Vector4unitY = { 0, 1, 0, 0 };
Vector4 const Vector4unitZ = { 0, 0, 1, 0 };
Vector4 const Vector4unitW = { 0, 0, 0, 1 };
Quaternion const Quaternion::Identity = math::Vector4unitW;

NS_END
