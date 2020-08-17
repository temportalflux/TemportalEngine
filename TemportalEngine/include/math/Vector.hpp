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
template <typename TValue, const ui8 TAccessibleDimensions, const ui8 TActualDimensions=TAccessibleDimensions>
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
	static_assert(TAccessibleDimensions <= TActualDimensions, "Dimension must be <= padded dimension");

protected:

	/** The format of this vector class, composed of the value and dimension. */
	typedef Vector<TValue, TAccessibleDimensions, TActualDimensions> VectorFormat;

private:

	/** The actual components/data of the vector. */
	TValue mValues[TActualDimensions];

public:

	/** The zero vector for this format. */
	static Vector<TValue, TAccessibleDimensions, TActualDimensions> const ZERO;

	constexpr Vector()
	{
		memset(mValues, 0, TActualDimensions * sizeof(TValue));
	}

	/**
	* @param values The array of the literal values of the vector.
	*	Length must match dimension of the vector.
	* Inverse of toArray(TValue[]).
	*/
	constexpr Vector(TValue values[TAccessibleDimensions])
	{
		memcpy_s(mValues, TAccessibleDimensions * sizeof(TValue), values, TAccessibleDimensions * sizeof(TValue));
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
		memcpy_s(mValues, TAccessibleDimensions * sizeof(TValue), other.mValues, TAccessibleDimensions * sizeof(TValue));
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
	template <ui32 TDimensionOther, ui8 TOtherPadding>
	constexpr Vector(Vector<TValue, TDimensionOther, TOtherPadding> const &other, ui8 offset = 0)
	{
		memset(mValues, 0, TAccessibleDimensions * sizeof(TValue));
		other.copyTo<TAccessibleDimensions>(mValues, offset);
	}

	// Operations: General ----------------------------------------------------

	Vector<f32, TAccessibleDimensions, TActualDimensions> toFloat() const
	{
		Vector<f32, TAccessibleDimensions, TActualDimensions> fVector;
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
			fVector[i] = (f32)mValues[i];
		return fVector;
	}

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
	template <ui8 TDimensionCountResult, ui8 TActualDimensionCountOther=TDimensionCountResult>
	constexpr Vector<TValue, TDimensionCountResult, TActualDimensionCountOther> const createSubvector(ui8 offset = 0) const
	{
		return Vector<TValue, TDimensionCountResult, TActualDimensionCountOther>(*this, offset);
	}

	/**
	* Copies data to an array with length equal to dimension count.
	* Inverse of Vector(TValue[]) constructor.
	*/
	template <ui8 TDimensionCountOther>
	constexpr void copyTo(TValue other[TDimensionCountOther], ui8 componentOffset = 0) const
	{
		ui8 destComponentCount = TDimensionCountOther - componentOffset;
		uSize copySize = math::min<ui8>(TAccessibleDimensions, destComponentCount) * sizeof(TValue);
		memcpy_s(other + componentOffset, copySize, mValues, copySize);
	}

	TValue* data()
	{
		return this->mValues;
	}

	/**
	* Set a value at a given dimension index. Can modify this value
	*	and the change will be reflected in the vector.
	*/
	constexpr TValue& operator[](ui8 const i)
	{
		assert(i < TAccessibleDimensions);
		return mValues[i];
	}

	/**
	* Get a value at a given dimension index. Cannot modify the value.
	*/
	constexpr TValue const & operator[](ui8 const i) const
	{
		assert(i < TAccessibleDimensions);
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
		static_assert(TAccessibleDimensions >= 1, "Cannot get X component of vector size < 1");
		return mValues[0];
	}

	TValue& x()
	{
		static_assert(TAccessibleDimensions >= 1, "Cannot get X component of vector size < 1");
		return mValues[0];
	}

	/**
	* Get the value at the second dimension (y).
	* Cannot modify this value.
	* Equivalent to vector[1].
	*/
	constexpr TValue const & y() const
	{
		static_assert(TAccessibleDimensions >= 2, "Cannot get Y component of vector size < 2");
		return mValues[1];
	}

	TValue& y()
	{
		static_assert(TAccessibleDimensions >= 2, "Cannot get Y component of vector size < 2");
		return mValues[1];
	}

	/**
	* Get the value at the third dimension (z).
	* Cannot modify this value.
	* Equivalent to vector[2].
	*/
	constexpr TValue const & z() const
	{
		static_assert(TAccessibleDimensions >= 3, "Cannot get Z component of vector size < 3");
		return mValues[2];
	}

	TValue& z()
	{
		static_assert(TAccessibleDimensions >= 3, "Cannot get Z component of vector size < 3");
		return mValues[2];
	}

	/**
	* Get the value at the fourth dimension (w).
	* Cannot modify this value.
	* Equivalent to vector[3].
	*/
	constexpr TValue const & w() const
	{
		static_assert(TAccessibleDimensions >= 4, "Cannot get W component of vector size < 4");
		return mValues[3];
	}

	TValue& w()
	{
		static_assert(TAccessibleDimensions >= 4, "Cannot get W component of vector size < 4");
		return mValues[3];
	}

	/**
	* Set the value at the first dimension (x).
	* Can modify this value.
	* Equivalent to vector[0].
	*/
	constexpr VectorFormat& x(TValue const &value)
	{
		static_assert(TAccessibleDimensions >= 1, "Cannot get X component of vector size < 1");
		mValues[0] = value;
		return *this;
	}

	/**
	* Set the value at the second dimension (y).
	* Can modify this value.
	* Equivalent to vector[1].
	*/
	constexpr VectorFormat& y(TValue const &value)
	{
		static_assert(TAccessibleDimensions >= 2, "Cannot get Y component of vector size < 2");
		mValues[1] = value;
		return *this;
	}

	/**
	* Set the value at the third dimension (z).
	* Can modify this value.
	* Equivalent to vector[2].
	*/
	constexpr VectorFormat& z(TValue const &value)
	{
		static_assert(TAccessibleDimensions >= 3, "Cannot get Z component of vector size < 3");
		mValues[2] = value;
		return *this;
	}

	/**
	* Set the value at the fourth dimension (w).
	* Can modify this value.
	* Equivalent to vector[3].
	*/
	constexpr VectorFormat& w(TValue const &value)
	{
		static_assert(TAccessibleDimensions >= 4, "Cannot get W component of vector size < 4");
		mValues[3] = value;
		return *this;
	}

	// Operators --------------------------------------------------------------

	bool operator==(VectorFormat const& other) const
	{
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
		{
			if (mValues[i] != other.mValues[i]) return false;
		}
		return true;
	}

	bool operator!=(VectorFormat const& other) const
	{
		return !(*this == other);
	}

	/**
	* Copy values from another vector of the same format.
	* If you want to copy values from a vector with a different dimension size,
	*	use a copy constructor.
	* Equivalent to using toArray(this)
	*/
	constexpr void operator=(VectorFormat const &other)
	{
		other.copyTo<TAccessibleDimensions>(mValues);
	}

	/**
	* Component wise adds another vector from this vector.
	*/
	constexpr void operator+=(VectorFormat const &other)
	{
		// TODO: There is definitely a way to apply scalar addition to a set of scalars in assembly in constant time
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
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
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
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
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
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
	 * Component wise divides another vector and this vector.
	 */
	constexpr void operator/=(VectorFormat const &other)
	{
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
			mValues[i] /= other.mValues[i];
	}

	/**
	 * Component wise divides another vector and this vector and returns the result.
	 */
	constexpr VectorFormat const operator/(VectorFormat const &other) const
	{
		VectorFormat ret = VectorFormat(*this);
		ret /= other;
		return ret;
	}

	/**
	* Multiplies this vector by a scalar.
	*/
	constexpr void operator*=(TValue const other)
	{
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
			mValues[i] *= other;
	}

	/**
	* Divides this vector by a scalar.
	*/
	constexpr void operator/=(TValue const other)
	{
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
			mValues[i] /= other;
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
	* Divides this vector by a scalar and returns the result.
	*/
	constexpr VectorFormat const operator/(TValue const other) const
	{
		VectorFormat ret = *this;
		ret /= other;
		return ret;
	}

	void operator%=(f32 const scalar)
	{
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
			mValues[i] %= scalar;
	}

	VectorFormat operator%(f32 const scalar) const
	{
		VectorFormat result = *this;
		result %= scalar;
		return result;
	}

	/**
	* Multiplies a vector by a scalar and returns the result.
	*/
	friend constexpr VectorFormat const operator*(
		TValue const scalar, VectorFormat const vector)
	{
		return vector * scalar;
	}

	math::Vector<i32, TAccessibleDimensions, TActualDimensions>
		removeExcess(i32 const scalar)
	{
		math::Vector<i32, TAccessibleDimensions, TActualDimensions> ret;
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
		{
			while (mValues[i] >= scalar)
			{
				ret[i] += scalar;
				mValues[i] -= scalar;
			}
			while (mValues[i] <= -scalar)
			{
				ret[i] -= scalar;
				mValues[i] += scalar;
			}
		}
		return ret;
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
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
			scalarProduct += mValues[i] * other.mValues[i];
		return scalarProduct;
	}

	/**
	* Calculates the cross product of two vectors of the same format.
	* @return l x r
	*/
	template <typename T = std::enable_if_t<TAccessibleDimensions == 3>>
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
	template <typename T = std::enable_if_t<TAccessibleDimensions == 3>>
	constexpr VectorFormat const crossRight(VectorFormat const &other) const
	{
		VectorFormat ret = VectorFormat::ZERO;
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
	template <typename T = std::enable_if_t<TAccessibleDimensions == 3>>
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
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
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
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
			mValues[i] = mValues[i] / magnitude;
	}

	// Returns the sum of all components
	TValue sum() const
	{
		TValue scalar = 0;
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
			scalar += mValues[i];
		return scalar;
	}

	/**
	 * Converts a unit vector to a bit mask where each dimension consumes 2 bits.
	 * Examples:
	 *   < 0, 0, 0, 0> = 00000000
	 *   < 1, 0, 0, 0> = 00000001
	 *   <-1, 0, 0, 0> = 00000010
	 *   < 0, 1, 0, 0> = 00000100
	 *   < 0,-1, 0, 0> = 00001000
	 *   < 0, 0, 1, 0> = 00010000
	 *   < 0, 0,-1, 0> = 00100000
	 *   < 0, 0, 0, 1> = 01000000
	 *   < 0, 0, 0,-1> = 10000000
	 *   < 1, 1, 1, 1> = 01010101
	 *   <-1,-1,-1,-1> = 10101010
	 * Treats all positives as 1 and all negatives as -1.
	 * Complexity: O(n) where n is the number of dimensions
	 */
	ui8 toBitMask() const
	{
		// ui8 is 8 bits, and with each dimension occupying 2 bits, this function can only handle 4 dimensions
		static_assert(TAccessibleDimensions <= 4, "BitMask only value for vectors of max size 4");
		ui8 mask = 0;
		for (ui8 i = 0; i < TAccessibleDimensions; ++i)
		{
			if (mValues[i] == 0) continue;
			mask |= (1 << ((i * 2) + (ui8)(mValues[i] < 0)));
		}
		return mask;
	}

};

class Quaternion : public Vector<f32, 4, 4>
{
public:
	static Quaternion const Identity;

	constexpr Quaternion() : Vector() {}
	constexpr Quaternion(f32 values[4]) : Vector(values) {}
	constexpr Quaternion(std::initializer_list<f32> values) : Vector(values) {}
	constexpr Quaternion(VectorFormat const &other) : Vector(other) {}

	template <ui32 TDimensionOther>
	constexpr Quaternion(Vector<f32, TDimensionOther> const &other) : Vector(other) {}

	static Quaternion FromAxisAngle(Vector<f32, 3> axis, f32 radians);
	static Quaternion FromEuler(Vector<f32, 3> euler);

	Vector<f32, 3> euler() const;
	Quaternion conjugate() const;
	Quaternion inverseQuat() const;

	// Performs the Hamilton Product to rotate first by `b` then by `a`.
	static Quaternion concat(Quaternion const &a, Quaternion const &b);

	// v' = q*v*q'
	Vector<f32, 3> rotate(Vector<f32, 3> const v) const;

};

typedef Vector<f32, 2, 2> Vector2;
typedef Vector<f32, 3, 3> Vector3;
typedef Vector<f32, 4, 4> Vector4;
typedef Vector<f32, 2, 4> Vector2Padded;
typedef Vector<f32, 3, 4> Vector3Padded;

typedef Vector<i32, 2, 2> Vector2Int;
typedef Vector<i32, 3, 3> Vector3Int;

typedef Vector<ui32, 2, 2> Vector2UInt;
typedef Vector<ui32, 3, 3> Vector3UInt;

extern Vector2 const Vector2unitX, Vector2unitY;
extern Vector3 const Vector3unitX, Vector3unitY, Vector3unitZ;
extern Vector4 const Vector4unitX, Vector4unitY, Vector4unitZ, Vector4unitW;
Quaternion const MultiplyVector(Vector3 const vector, Quaternion const quat);

// TODO: This should really be in a physics thing, not a math thing
Quaternion const IntegrateKinematic(Quaternion const rotation,
	Vector3 const angularVelocity, Vector3 const angularAcceleration, f32 const deltaTime);

NS_END

#endif