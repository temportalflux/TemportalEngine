#pragma once

#include "TemportalEnginePCH.hpp"

NS_GRAPHICS

template <typename TVulkan, typename TWrapped>
TVulkan& extract(TWrapped *ptr)
{
	return *reinterpret_cast<TVulkan*>(ptr->get());
}

template <typename TVulkan, typename TWrapped>
TVulkan& extract(TWrapped const* ptr)
{
	return *reinterpret_cast<TVulkan*>(ptr->get());
}

NS_END
