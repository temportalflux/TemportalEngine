#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"

NS_MATH

template <typename TValue, ui8 Width, ui8 Height>
class Matrix
{
	typedef Vector<TValue, Width> TRow;
	typedef Vector<TValue, Height> TColumn;
	typedef Matrix<TValue, Width, Height> TMatrix;

public:
	Matrix()
	{
		memset(mColumns, 0, sizeof(TValue) * Width * Height);
	}

	Matrix(TMatrix const& other)
	{
		uSize size = sizeof(TValue) * Width * Height;
		memcpy_s(mColumns, size, other.mColumns, size);
	}

	Matrix(TValue identityMultiplier) : Matrix()
	{
		assert(Width == Height);
		for (ui8 idx = 0; idx < Width; ++idx)
			this->mColumns[idx][idx] = identityMultiplier;
	}

	void* data() { return &this->mColumns; }

	/**
	 * Returns the column for the specified index.
	 */
	constexpr TColumn& operator[](ui8 const idx)
	{
		assert(idx < Width);
		return this->mColumns[idx];
	}

	TMatrix& setRow(ui8 const idxRow, TRow const &row)
	{
		for (ui8 idx = 0; idx < Width; ++idx)
			this->mColumns[idx][idxRow] = row[idx];
		return *this;
	}

	TMatrix& operator*=(TMatrix const& other);

	TMatrix operator*(TMatrix const &other) const
	{
		TMatrix result = TMatrix(*this);
		result *= other;
		return result;
	}

private:
	TColumn mColumns[Width];

};

typedef Matrix<f32, 4, 4> Matrix4x4;

Matrix4x4 translate(Vector3 const& translation);
Matrix4x4 toMatrix(Quaternion const& quat);
Matrix4x4 scale(Vector3 const& scale);
Matrix4x4 createModelMatrix(Vector3 const& translation, Quaternion const& rotation, Vector3 const& scale);

NS_END
