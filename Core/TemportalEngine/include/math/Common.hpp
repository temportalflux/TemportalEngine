#pragma once

#include "TemportalEnginePCH.hpp"

NS_MATH

template <typename V, typename T>
constexpr V lerp(V const& a, V const& b, T const& t)
{
	return a*(V(1)-t) + b*(t);
}

NS_END
