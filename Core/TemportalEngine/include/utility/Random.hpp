#pragma once

#include "TemportalEnginePCH.hpp"

NS_UTILITY

class Random
{
public:
	Random(ui32 seed);

	i32 next();
	i32 nextIn(i32 min, i32 max);

	template <uSize TDim>
	math::Vector<i32, TDim> nextV()
	{
		auto v = math::Vector<i32, TDim>(0);
		for (uIndex i = 0; i < TDim; ++i) v[i] = this->next();
		return v;
	}

	template <uSize TDim>
	math::Vector<i32, TDim> nextVInV(
		math::Vector<i32, TDim> const& min,
		math::Vector<i32, TDim> const& max
	)
	{
		auto v = math::Vector<i32, TDim>(0);
		for (ui8 i = 0; i < TDim; ++i)
			v[i] = this->nextIn(min[i], max[i]);
		return v;
	}

	template <uSize TDim>
	math::Vector<i32, TDim> nextVIn(i32 min, i32 max)
	{
		return this->nextVInV<TDim>(
			math::Vector<i32, TDim>(min),
			math::Vector<i32, TDim>(max)
		);
	}

	template <uSize TDim>
	math::Vector<i32, TDim> nextVInS(i32 uniformRange)
	{
		return this->nextVIn<TDim>(-uniformRange, uniformRange);
	}

};

NS_END
