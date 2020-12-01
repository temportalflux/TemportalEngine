#pragma once

#include "CoreInclude.hpp"

template <typename TValue, uSize TSize>
class CoordMap
{

public:
	typedef math::Vector3UInt TCoordinate;

	TValue const& operator[](TCoordinate const coord) const
	{
		assert(coord.x() < TSize && coord.y() < TSize && coord.z() < TSize);
		return mValues[coord.z()][coord.y()][coord.x()];
	}

	TValue& operator[](TCoordinate const coord)
	{
		assert(coord.x() < TSize && coord.y() < TSize && coord.z() < TSize);
		return mValues[coord.z()][coord.y()][coord.x()];
	}

private:
	std::array<std::array<std::array<TValue, TSize>, TSize>, TSize> mValues;
	
};
