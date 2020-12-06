#include "math/Matrix.hpp"

math::Matrix4x4 math::translate(Vector3 const& translation)
{
	auto identity = math::Matrix4x4(1);
	auto result = math::Matrix4x4(1);
	result[3] = (identity[0] * translation[0]) + (identity[1] * translation[1]) + (identity[2] * translation[2]) + identity[3];
	return result;
}

math::Matrix4x4 math::toMatrix(Quaternion const& quat)
{
	auto result = math::Matrix4x4(1);

	f32 qxx(quat.x() * quat.x());
	f32 qyy(quat.y() * quat.y());
	f32 qzz(quat.z() * quat.z());
	f32 qxz(quat.x() * quat.z());
	f32 qxy(quat.x() * quat.y());
	f32 qyz(quat.y() * quat.z());
	f32 qwx(quat.w() * quat.x());
	f32 qwy(quat.w() * quat.y());
	f32 qwz(quat.w() * quat.z());

	result[0][0] = f32(1) - f32(2) * (qyy + qzz);
	result[0][1] = f32(2) * (qxy + qwz);
	result[0][2] = f32(2) * (qxz - qwy);

	result[1][0] = f32(2) * (qxy - qwz);
	result[1][1] = f32(1) - f32(2) * (qxx + qzz);
	result[1][2] = f32(2) * (qyz + qwx);

	result[2][0] = f32(2) * (qxz + qwy);
	result[2][1] = f32(2) * (qyz - qwx);
	result[2][2] = f32(1) - f32(2) * (qxx + qyy);

	return result;
}

math::Matrix4x4 math::scale(Vector3 const& scale)
{
	auto identity = math::Matrix4x4(1);
	auto scaling = math::Matrix4x4(1);
	scaling[0] = identity[0] * scale[0];
	scaling[1] = identity[1] * scale[1];
	scaling[2] = identity[2] * scale[2];
	scaling[3] = identity[3];
	return scaling;
}

math::Matrix4x4 math::createModelMatrix(Vector3 const& translation, Quaternion const& rotation, Vector3 const& scale)
{
	auto result = math::translate(translation);
	result *= math::toMatrix(rotation);
	result *= math::scale(scale);
	return result;
}

template <>
math::Matrix4x4& math::Matrix4x4::operator*=(math::Matrix4x4 const& other)
{
	auto self = math::Matrix4x4(*this);

	math::Vector4 const& BC0 = other.mColumns[0];
	math::Vector4 const& BC1 = other.mColumns[1];
	math::Vector4 const& BC2 = other.mColumns[2];
	math::Vector4 const& BC3 = other.mColumns[3];

	mColumns[0] = self.mColumns[0] * BC0[0] + self.mColumns[1] * BC0[1] + self.mColumns[2] * BC0[2] + self.mColumns[3] * BC0[3];
	mColumns[1] = self.mColumns[0] * BC1[0] + self.mColumns[1] * BC1[1] + self.mColumns[2] * BC1[2] + self.mColumns[3] * BC1[3];
	mColumns[2] = self.mColumns[0] * BC2[0] + self.mColumns[1] * BC2[1] + self.mColumns[2] * BC2[2] + self.mColumns[3] * BC2[3];
	mColumns[3] = self.mColumns[0] * BC3[0] + self.mColumns[1] * BC3[1] + self.mColumns[2] * BC3[2] + self.mColumns[3] * BC3[3];

	return *this;
}
