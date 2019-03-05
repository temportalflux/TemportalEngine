#ifndef MATH_VECTOR_HPP
#define MATH_VECTOR_HPP

#include "TemportalEnginePCH.hpp"

// TODO: Organize Headers

#include "math/VectorType.hpp"
#include "types/integer.h"
#include "types/real.h"

#include <string.h> // memset/memcpy
#include <type_traits>
#include <cmath>
#include <cassert>
#include <algorithm>

NS_MATH

template <typename TValue, ui32 TValueCount>
class TEMPORTALENGINE_API Vector
{
	static_assert(
		std::is_same<TValue, f32>::value
		|| std::is_same<TValue, f64>::value
		|| std::is_same<TValue, i8>::value
		|| std::is_same<TValue, ui8>::value
		|| std::is_same<TValue, i32>::value
		|| std::is_same<TValue, ui32>::value,
		"Invalid Vector value type"
	);

	typedef Vector<TValue, TValueCount> Self;

private:

	TValue mValues[TValueCount];

public:
	static Vector const zero;

	Vector(TValue values[TValueCount])
	{
		memcpy_s(mValues, TValueCount, values, TValueCount);
	}

	Vector()
	{
		memset(mValues, 0, TValueCount);
	}

	Vector(std::initializer_list<TValue> values)
	{
		std::copy(values.begin(), values.end(), mValues);
	}

	Vector(Self const &other)
	{
		memcpy_s(mValues, TValueCount, other.mValues, TValueCount);
	}

	template <ui32 TValueCountOther>
	Vector(Vector<TValue, TValueCountOther> const &other)
	{
		memset(mValues, 0, TValueCount);
		other.getValues(mValues);
	}

	void getValues(TValue out[TValueCount]) const
	{
		memcpy_s(out, TValueCount, mValues, TValueCount);
	}

	TValue& operator[](ui32 const i)
	{
		assert(i < TValueCount);
		return mValues[i];
	}

	TValue const & operator[](ui32 const i) const
	{
		assert(i < TValueCount);
		return mValues[i];
	}

	// ------------------------------------------------------------------------
	// Properties

	TValue const x() const
	{
		static_assert(TValueCount >= 1, "Cannot get X component of vector size < 1");
		return mValues[0];
	}

	TValue const x(TValue const &value)
	{
		static_assert(TValueCount >= 1, "Cannot get X component of vector size < 1");
		mValues[0] = value;
		return mValues[0];
	}

	TValue const y() const
	{
		static_assert(TValueCount >= 2, "Cannot get Y component of vector size < 2");
		return mValues[1];
	}

	TValue const y(TValue const &value)
	{
		static_assert(TValueCount >= 2, "Cannot get Y component of vector size < 2");
		mValues[1] = value;
		return mValues[1];
	}

	TValue const z() const
	{
		static_assert(TValueCount >= 3, "Cannot get Z component of vector size < 3");
		return mValues[2];
	}

	TValue const z(TValue const &value)
	{
		static_assert(TValueCount >= 3, "Cannot get Z component of vector size < 3");
		mValues[2] = value;
		return mValues[2];
	}

	TValue const w() const
	{
		static_assert(TValueCount >= 4, "Cannot get W component of vector size < 4");
		return mValues[3];
	}

	TValue const w(TValue const &value)
	{
		static_assert(TValueCount >= 4, "Cannot get W component of vector size < 4");
		mValues[3] = value;
		return mValues[3];
	}

	template <ui8 TSubCount>
	Vector<TValue, TSubCount> const subvector() const
	{
		return Vector<TValue, TSubCount>(*this);
	}

	// ------------------------------------------------------------------------
	// Generic Properties

	static TValue const dot(Self const &a, Self const &b)
	{
		TValue sum = 0;
		for (ui8 i = 0; i < TValueCount; ++i)
			sum += a.mValues[i] * b.mValues[1];
		return sum;
	}

	static Self cross(Self const &a, Self const &b)
	{
		Self ret = Self();
		if (TValueCount != 3) return ret;
		ret.mValues[0] = a.mValues[1] * b.mValues[2] - a.mValues[2] * b.mValues[1];
		ret.mValues[1] = a.mValues[2] * b.mValues[0] - a.mValues[0] * b.mValues[2];
		ret.mValues[2] = a.mValues[0] * b.mValues[1] - a.mValues[1] * b.mValues[0];
		return ret;
	}

	TValue const magnitudeSq() const
	{
		return Self::dot(*this, *this);
	}

	TValue const magnitude() const
	{
		// NOTE: This is an expensive operation
		return std::sqrt(this->magnitudeSq());
	}

	Self operator=(Self const &other)
	{
		other.getValues(mValues);
	}

	Self operator+(Self const &other) const
	{
		Self ret = Self(*this);
		ret += other;
		return ret;
	}

	void operator+=(Self const &other)
	{
		for (ui8 i = 0; i < TValueCount; ++i)
			mValues[i] += other.mValues[i];
	}

	Self operator-(Self const &other) const
	{
		Self ret = Self(*this);
		ret -= other;
		return ret;
	}

	void operator-=(Self const &other)
	{
		for (ui8 i = 0; i < TValueCount; ++i)
			mValues[i] -= other.mValues[i];
	}

	Self operator*(Self const &other) const
	{
		Self ret = Self(*this);
		ret *= other;
		return ret;
	}

	void operator*=(Self const &other)
	{
		for (ui8 i = 0; i < TValueCount; ++i)
			mValues[i] *= other.mValues[i];
	}

	Self operator*(TValue const other) const
	{
		Self ret = *this;
		//ret *= other;
		return ret;
	}

	void operator*=(TValue const other)
	{
		for (ui8 i = 0; i < TValueCount; ++i)
			mValues[i] *= other;
	}

	friend Self operator*(TValue const scalar, Self const vector)
	{
		return vector * scalar;
	}

	Self operator-() const
	{
		return inverse();
	}

	Self inverse() const
	{
		Self ret = Self(*this);
		ret.invert();
		return ret;
	}

	void invert()
	{
		// NOTE: Potentially expensive operation
		for (ui8 i = 0; i < TValueCount; ++i)
			mValues[i] = 1 / mValues[i];
	}

	Self normalized() const
	{
		Self ret = Self(*this);
		ret.normalize();
		return ret;
	}

	void normalize()
	{
		// NOTE: Potentially expensive operation
		TValue magnitude = this->magnitude();
		for (ui8 i = 0; i < TValueCount; ++i)
			mValues[i] = mValues[i] / magnitude;
	}

};

typedef Vector<f32, 2> Vector2;
typedef Vector<f32, 3> Vector3;
typedef Vector<f32, 4> Vector4;
typedef Vector4 Quaternion;

typedef Vector<i32, 2> Vector2Int;
typedef Vector<i32, 3> Vector3Int;

typedef Vector<ui32, 2> Vector2UInt;
typedef Vector<ui32, 3> Vector3UInt;

Vector2 const Vector2unitX, Vector2unitY;
Vector3 const Vector3unitX, Vector3unitY, Vector3unitZ;
Vector4 const Vector4unitX, Vector4unitY, Vector4unitZ, Vector4unitW;
Quaternion const QuaternionIdentity;

Quaternion const QuaternionFromAxisAngle(Vector3 const axis, float const angleRad);
Quaternion const QuaternionConjugate(Quaternion const quat);
Quaternion const QuaternionInverse(Quaternion const quat);
Quaternion const QuaternionConcatenate(Quaternion const a, Quaternion const b);
Vector3 const RotateVector(Vector3 const vector, Quaternion const rotation);
Quaternion const MultiplyVector(Vector3 const vector, Quaternion const quat);

// TODO: This should really be in a physics thing, not a math thing
Quaternion const IntegrateKinematic(Quaternion const rotation,
	Vector3 const angularVelocity, Vector3 const angularAcceleration, f32 const deltaTime);

NS_END

#endif