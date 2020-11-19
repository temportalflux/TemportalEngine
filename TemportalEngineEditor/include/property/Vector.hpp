#pragma once

#include "property/Number.hpp"

NS_PROPERTIES

template <typename TValue, const ui8 TDimensions, const ui8 TPaddedDimensions = TDimensions>
struct PropertyVector
	: public PropertyMinimal<math::Vector<TValue, TDimensions, TPaddedDimensions>>
	, public NumberSettings<TValue>
{
	typedef math::Vector<TValue, TDimensions, TPaddedDimensions> TVector;

	PropertyVector() : PropertyMinimal<TVector>() {}
	
	PropertyVector(
		TVector initial, TVector value,
		std::optional<TValue> step = std::nullopt, std::optional<TValue> stepFast = std::nullopt
	) : PropertyMinimal<TVector>(initial, value)
	{
		this->step = step;
		this->stepFast = stepFast;
	}

};

template <typename TValue, const ui8 TDimensions, const ui8 TPaddedDimensions = TDimensions>
bool renderPropertyEditor(const char* id, PropertyVector<TValue, TDimensions, TPaddedDimensions> &prop)
{
	return properties::renderNumbers<TValue>(id, prop.initial.data(), prop.value.data(), TDimensions, prop);
}

NS_END
