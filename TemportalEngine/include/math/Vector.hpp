#ifndef MATH_VECTOR2_HPP
#define MATH_VECTOR2_HPP

#include "Namespace.h"
#include "Api.h"
#include "math/Vector.h"
#include "math/VectorType.hpp"

#include <string.h> // memset/memcpy
#include <type_traits>
#include <cmath>
#include <cassert>

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

	template <typename ... T>
	Vector(T... values)
		: mValues{ (TValue)values... }
	{
	}

	Vector(Vector<TValue, TValueCount> const &other)
		: Vector(other.mValues)
	{ }

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

typedef Vector<i32, 2> Vector2Int;
typedef Vector<i32, 3> Vector3Int;

typedef Vector<ui32, 2> Vector2UInt;
typedef Vector<ui32, 3> Vector3UInt;

Vector2 const Vector2unitX, Vector2unitY;
Vector3 const Vector3unitX, Vector3unitY, Vector3unitZ;
Vector4 const Vector4unitX, Vector4unitY, Vector4unitZ, Vector4unitW;

NS_END

#endif