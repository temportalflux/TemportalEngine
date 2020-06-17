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

	/** The actual components/data of the vector. */
	TValue mValues[TDimension];

public:

	/** The zero vector for this format. */
	static Vector<TValue, TDimension> const ZERO;

	constexpr Vector()
	{
		memset(mValues, 0, TDimension * sizeof(TValue));
	}

	/**
	* @param values The array of the literal values of the vector.
	*	Length must match dimension of the vector.
	* Inverse of toArray(TValue[]).
	*/
	constexpr Vector(TValue values[TDimension])
	{
		memcpy_s(mValues, TDimension * sizeof(TValue), values, TDimension * sizeof(TValue));
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
		memcpy_s(mValues, TDimension * sizeof(TValue), other.mValues, TDimension * sizeof(TValue));
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
		memset(mValues, 0, TDimension * sizeof(TValue));
		other.copyTo<TDimension>(mValues);
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
	template <ui32 TDimensionOther>
	constexpr void copyTo(TValue other[TDimensionOther], ui32 componentOffset = 0) const
	{
		uSize destComponentCount = TDimensionOther + componentOffset;
		uSize copySize = (TDimension < destComponentCount ? TDimension : destComponentCount) * sizeof(TValue);
		memcpy_s(other + componentOffset, copySize, mValues + componentOffset, copySize);
	}

	TValue* data()
	{
		return this->mValues;
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

	// Operators --------------------------------------------------------------

	/**
	* Copy values from another vector of the same format.
	* If you want to copy values from a vector with a different dimension size,
	*	use a copy constructor.
	* Equivalent to using toArray(this)
	*/
	constexpr void operator=(VectorFormat const &other)
	{
		other.copyTo<TDimension>(mValues);
	}

	/**
	* Component wise adds another vector from this vector.
	*/
	constexpr void operator+=(VectorFormat const &other)
	{
		// TODO: There is definitely a way to apply scalar addition to a set of scalars in assembly in constant time
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] += other.mValues[i];
	}

	/**
	* Component wise adds another vector to this vector
	*	and returns the result.
	*/
	constexpr VectorFormat const operator+(VectorFormat const &other) const
	{
		VectorFormat ret = VectorFormat(*this);
		ret += other;
		return ret;
	}

	/**
	* Component wise subtracts another vector from this vector.
	*/
	constexpr void operator-=(VectorFormat const &other)
	{
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] -= other.mValues[i];
	}

	/**
	* Component wise subtracts another vector from this vector
	*	and returns the result.
	*/
	constexpr VectorFormat const operator-(VectorFormat const &other) const
	{
		VectorFormat ret = VectorFormat(*this);
		ret -= other;
		return ret;
	}

	/**
	* Component wise multiplies another vector and this vector.
	*/
	constexpr void operator*=(VectorFormat const &other)
	{
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] *= other.mValues[i];
	}

	/**
	* Component wise multiplies another vector and this vector
	*	and returns the result.
	*/
	constexpr VectorFormat const operator*(VectorFormat const &other) const
	{
		VectorFormat ret = VectorFormat(*this);
		ret *= other;
		return ret;
	}

	/**
	* Multiplies this vector by a scalar.
	*/
	constexpr void operator*=(TValue const other)
	{
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] *= other;
	}

	/**
	* Multiplies this vector by a scalar and returns the result.
	*/
	constexpr VectorFormat const operator*(TValue const other) const
	{
		VectorFormat ret = *this;
		ret *= other;
		return ret;
	}

	/**
	* Multiplies a vector by a scalar and returns the result.
	*/
	friend constexpr VectorFormat const operator*(
		TValue const scalar, VectorFormat const vector)
	{
		return vector * scalar;
	}

	/**
	* Calculates the scalar (dot) product of two vectors of the same format.
	*/
	static constexpr TValue const dot(VectorFormat const &a, VectorFormat const &b)
	{
		return a.dot(b);
	}

	/**
	* Calculates the scale (dot) product of this and another vector of the same format.
	*/
	constexpr TValue const dot(VectorFormat const &other) const
	{
		TValue scalarProduct = 0;
		for (ui8 i = 0; i < TDimension; ++i)
			scalarProduct += mValues[i] * other.mValues[1];
		return scalarProduct;
	}

	/**
	* Calculates the cross product of two vectors of the same format.
	* @return l x r
	*/
	static constexpr VectorFormat const cross(VectorFormat const &l, VectorFormat const &r)
	{
		return l.crossRight(r);
	}

	/**
	* Calculates the cross product of this and another vector of the same format.
	* {x1, y1, z1} x {x2, y2, z2} = {y1z2 - z1y2, z1x2 - x1z2, x1y2 - y1x2}
	* @param other The other vector
	* @return this x other
	*/
	constexpr VectorFormat const crossRight(VectorFormat const &other) const
	{
		VectorFormat ret = VectorFormat::ZERO;
		if (TDimension != 3) return ret;
		ret.mValues[0] = mValues[1] * other.mValues[2] - mValues[2] * other.mValues[1];
		ret.mValues[1] = mValues[2] * other.mValues[0] - mValues[0] * other.mValues[2];
		ret.mValues[2] = mValues[0] * other.mValues[1] - mValues[1] * other.mValues[0];
		return ret;
	}

	/**
	* Calculates the cross product of this and another vector of the same format.
	* @param other The other vector
	* @return other x this
	*/
	constexpr VectorFormat const crossLeft(VectorFormat const &other) const
	{
		return other.crossRight(*this);
	}

	/**
	* Calculates the square magnitude of a vector (length * length).
	* This is preferable to magnitude() as it does not perform a square root.
	*/
	constexpr TValue const magnitudeSq() const
	{
		return VectorFormat::dot(*this, *this);
	}

	/**
	* Calculates the magnitude of a vector (its length).
	*/
	constexpr TValue const magnitude() const
	{
		// NOTE: This is an expensive operation
		return std::sqrt(this->magnitudeSq());
	}
	
	/**
	* Calculates the component-wise negation of this vector and returns the result.
	* Equivalent to calling inverse().
	*/
	constexpr VectorFormat const operator-() const
	{
		return inverse();
	}

	/**
	* Calculates the component-wise negation of this vector and returns the result.
	*/
	constexpr VectorFormat const inverse() const
	{
		VectorFormat ret = VectorFormat(*this);
		ret.invert();
		return ret;
	}

	/**
	* Component-wise negates this vector.
	*/
	void invert()
	{
		// NOTE: Potentially expensive operation
		for (ui8 i = 0; i < TDimension; ++i)
			mValues[i] = -mValues[i];
	}

	/**
	* Calculates this vector as a unit vector and returns the result.
	*/
	VectorFormat normalized() const
	{
		VectorFormat ret = VectorFormat(*this);
		ret.normalize();
		return ret;
	}

	/**
	* Turns this vector into a unit vector with the same direction (magnitude will equal 1).
	*/
	void normalize()
	{
		// NOTE: Potentially expensive operation
		TValue const magnitude = this->magnitude();
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

extern Vector2 const Vector2unitX, Vector2unitY;
extern Vector3 const Vector3unitX, Vector3unitY, Vector3unitZ;
extern Vector4 const Vector4unitX, Vector4unitY, Vector4unitZ, Vector4unitW;
extern Quaternion const QuaternionIdentity;

Quaternion const QuaternionFromAxisAngle(Vector3 const axis, float const angleRad);
Quaternion const QuaternionConjugate(Quaternion const quat);
Quaternion const QuaternionInverse(Quaternion const quat);
// Performs the Hamilton Product to rotate first by `b` then by `a`.
Quaternion const QuaternionConcatenate(Quaternion const &a, Quaternion const &b);
Vector3 const RotateVector(Vector3 const vector, Quaternion const rotation);
Quaternion const MultiplyVector(Vector3 const vector, Quaternion const quat);

// TODO: This should really be in a physics thing, not a math thing
Quaternion const IntegrateKinematic(Quaternion const rotation,
	Vector3 const angularVelocity, Vector3 const angularAcceleration, f32 const deltaTime);

NS_END

#endif