#pragma once

#include "property/Number.hpp"

NS_PROPERTIES

template <typename TValue, const ui8 TDimensions, const ui8 TPaddedDimensions = TDimensions>
PropertyResult renderPropertyEditor(
	const char* id,
	math::Vector<TValue, TDimensions, TPaddedDimensions> &value,
	math::Vector<TValue, TDimensions, TPaddedDimensions> const& defaultValue
)
{
	return properties::renderNumbers<TValue>(id, defaultValue.data(), value.data(), TDimensions);
}

NS_END
