#ifndef MATH_VECTOR_HPP
#define MATH_VECTOR_HPP

// PCH ------------------------------------------------------------------------
#include "TemportalEnginePCH.hpp"

// Libraries ------------------------------------------------------------------
#include <algorithm>
#include <cassert>
#include <cmath>
#include <string.h> // memset/memcpy
#include <type_traits>

// Engine ---------------------------------------------------------------------
#include "math/VectorType.hpp"
#include "types/integer.h"
#include "types/real.h"

// ----------------------------------------------------------------------------
NS_MATH

/**
* Helper class for vector math. Staticly defined constant memory using a given
* numerical data type and given dimension
* (traditional vectors are 3-dimensional float vectors - Vector<float, 3>).
* @param TValue The data type (floats and signed/unsigned ints supported).
* @param TDimension The number of components.
*/
template <typename TValue, ui32 TDimension>
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

	/** The format of this vector class, composed of the value and dimension. */
	typedef Vector<TValue, TDimension> VectorFormat;

private:

	// NOTE: Not using std::array due to linker issues.
	/** The actual components/data of the vector. */
	TValue mValues[TDimension];

public:

	/** The zero vector for this format. */
	static Vector const ZERO;

	constexpr Vector()
	{
		memset(mValues, 0, TDimension);
	}

	/**
	* @param values The array of the literal values of the vector.
	*	Length must match dimension of the vector.
	* Inverse of toArray(TValue[]).
	*/
	constexpr Vector(TValue values[TDimension])
	{
		memcpy_s(mValues, TDimension, values, TDimension);
	}

	/**
	* Initialized using the standard initializer list format
	*/
	constexpr Vector(std::initializer_list<TValue> values)
	{
		std::copy(values.begin(), values.end(), mValues);
	}

	/**
	* Copy constructor to duplicate a vector of the same format.
	*/
	constexpr Vector(VectorFormat const &other)
	{
		memcpy_s(mValues, TDimension, other.mValues, TDimension);
	}

	/**
	* Copy constructor to duplicate a vector
	*	of the same data type but a different dimension.
	* If TDimensionOther < TDimension, then this vector will contain
	*	all of the other vector, with 0 padding in the remaining dimensions.
	* If TDimensionOther > TDimension, then this vector will contain
	*	the first TDimension values of the other vector.
	* Otherwise this is a copy equivalent to Vector(VectorFormat).
	*	(It wont cause a compiler error because this constructor
	*	takes the TDimensionOther parameter.
	* Inverse of createSubvector().
	* @param TDimensionResult The dimension of the vector being copied.
	*/
	template <ui32 TDimensionOther>
	constexpr Vector(Vector<TValue, TDimensionOther> const &other)
	{
		memset(mValues, 0, TDimension);
		other.toArray(mValues);
	}

	// Operations: General ----------------------------------------------------

	/**
	* Copies this vector into a new vector with a different dimension count.
	* If TDimensionResult < TDimension, then the new vector will contain
	*	all of this vector, with 0 padding in the remaining dimensions.
	* If TDimensionResult > TDimension, then the new vector will contain
	*	the first TDimension values of this vector.
	* Otherwise this is a copy equivalent to Vector(VectorFormat).
	* Inverse of Vector(Vector<TValue, TDimensionResult>).
	* This is literally calling the copy constructor.
	* @param TDimensionResult The dimension of the vector being copied.
	*/
	template <ui8 TDimensionResult>
	constexpr Vector<TValue, TDimensionResult> const createSubvector() const
	{
		return Vector<TValue, TDimensionResult>(*this);
	}

	/**
	* Copies data to an array with length equal to dimension count.
	* Inverse of Vector(TValue[]) constructor.
	*/
	constexpr void toArray(TValue out[TDimension]) const
	{
		memcpy_s(out, TDimension, mValues, TDimension);
	}

	/**
	* Set a value at a given dimension index. Can modify this value
	*	and the change will be reflected in the vector.
	*/
	constexpr TValue& operator[](ui32 const i)
	{
		assert(i < TDimension);
		return mValues[i];
	}

	/**
	* Get a value at a given dimension index. Cannot modify the value.
	*/
	constexpr TValue const & operator[](ui32 const i) const
	{
		assert(i < TDimension);
		return mValues[i];
	}

	// Dimension getter/setters -----------------------------------------------

	/**
	* Get the value at the first dimension (x).
	* Cannot modify this value.
	* Equivalent to vector[0].
	*/
	constexpr TValue const & x() const
	{
		static_assert(TDimension >= 1, "Cannot get X component of vector size < 1");
		return mValues[0];
	}

	/**
	* Get the value at the second dimension (y).
	* Cannot modify this value.
	* Equivalent to vector[1].
	*/
	constexpr TValue const & y() const
	{
		static_assert(TDimension >= 2, "Cannot get Y component of vector size < 2");
		return mValues[1];
	}

	/**
	* Get the value at the third dimension (z).
	* Cannot modify this value.
	* Equivalent to vector[2].
	*/
	constexpr TValue const & z() const
	{
		static_assert(TDimension >= 3, "Cannot get Z component of vector size < 3");
		return mValues[2];
	}

	/**
	* Get the value at the fourth dimension (w).
	* Cannot modify this value.
	* Equivalent to vector[3].
	*/
	constexpr TValue const & w() const
	{
		static_assert(TDimension >= 4, "Cannot get W component of vector size < 4");
		return mValues[3];
	}

	/**
	* Set the value at the first dimension (x).
	* Can modify this value.
	* Equivalent to vector[0].
	*/
	constexpr TValue& x(TValue const &value)
	{
		static_assert(TDimension >= 1, "Cannot get X component of vector size < 1");
		mValues[0] = value;
		return mValues[0];
	}

	/**
	* Set the value at the second dimension (y).
	* Can modify this value.
	* Equivalent to vector[1].
	*/
	constexpr TValue& y(TValue const &value)
	{
		static_assert(TDimension >= 2, "Cannot get Y component of vector size < 2");
		mValues[1] = value;
		return mValues[1];
	}

	/**
	* Set the value at the third dimension (z).
	* Can modify this value.
	* Equivalent to vector[2].
	*/
	constexpr TValue& z(TValue const &value)
	{
		static_assert(TDimension >= 3, "Cannot get Z component of vector size < 3");
		mValues[2] = value;
		return mValues[2];
	}

	/**
	* Set the value at the fourth dimension (w).
	* Can modify this value.
	* Equivalent to vector[3].
	*/
	constexpr TValue& w(TValue const &value)
	{
		static_assert(TDimension >= 4, "Cannot get W component of vector size < 4");
		mValues[3] = value;
		return mValues[3];
	}

	// ------------------------------------------------------------------------
	// Generic Properties

	// TODO: Document and constexpr remaining vector functions

	static TValue const dot(VectorFormat const &a, VectorFormat const &b)
	{
		TValue sum = 0;
		for (ui8 i = 0; i < TDimension; ++i)
			sum += a.mValues[i] * b.mValues[1];
		return sum;
	}

	static VectorFormat cross(VectorFormat const &a, VectorFormat const &b)
	{
		VectorFormat ret = VectorFormat();
		if (TDimension != 3) return ret;
		ret.mValues[0] = a.mValues[1] * b.mValues[2] - a.mValues[2] * b.mValues[1];
		ret.mValues[1] = a.mValues[2] * b.mValues[0] - a.mValues[0] * b.mValues[2];
		ret.mValues[2] = a.mValues[0] * b.mValues[1] - a.mValues[1] * b.mValues[0];
		return ret;
	}

	TValue const magnitudeSq() const
	{
		return VectorFormat::dot(*this, *this);
	}

	TValue const magnitude() const
	{
		// NOTE: This is an expensive operation
		return std::sqrt(this->magnitudeSq());
	}

	VectorFormat operator=(VectorFormat const &other)
	{
		other.toArray(mValues);
	}

	VectorFormat operator+(VectorFormat const &other) const
	{
		VectorFormat ret = VectorFormat(*this);
		ret += other;
		return ret;
	}

	void operator+=(VectorFormat const &other)
	{
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] += other.mValues[i];
	}

	VectorFormat operator-(VectorFormat const &other) const
	{
		VectorFormat ret = VectorFormat(*this);
		ret -= other;
		return ret;
	}

	void operator-=(VectorFormat const &other)
	{
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] -= other.mValues[i];
	}

	VectorFormat operator*(VectorFormat const &other) const
	{
		VectorFormat ret = VectorFormat(*this);
		ret *= other;
		return ret;
	}

	void operator*=(VectorFormat const &other)
	{
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] *= other.mValues[i];
	}

	VectorFormat operator*(TValue const other) const
	{
		VectorFormat ret = *this;
		//ret *= other;
		return ret;
	}

	void operator*=(TValue const other)
	{
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] *= other;
	}

	friend VectorFormat operator*(TValue const scalar, VectorFormat const vector)
	{
		return vector * scalar;
	}

	VectorFormat operator-() const
	{
		return inverse();
	}

	VectorFormat inverse() const
	{
		VectorFormat ret = VectorFormat(*this);
		ret.invert();
		return ret;
	}

	void invert()
	{
		// NOTE: Potentially expensive operation
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] = 1 / mValues[i];
	}

	VectorFormat normalized() const
	{
		VectorFormat ret = VectorFormat(*this);
		ret.normalize();
		return ret;
	}

	void normalize()
	{
		// NOTE: Potentially expensive operation
		TValue magnitude = this->magnitude();
		for (ui8 i = 0; i < TDimension; ++i)
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