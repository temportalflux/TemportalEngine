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
	Matrix(TValue identityMultiplier) : Matrix()
	{
		assert(Width == Height);
		for (ui8 idx = 0; idx < Width; ++idx)
			this->mColumns[idx][idx] = identityMultiplier;
	}

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

private:
	TColumn mColumns[Width];

};

typedef Matrix<f32, 4, 4> Matrix4x4;

NS_END
