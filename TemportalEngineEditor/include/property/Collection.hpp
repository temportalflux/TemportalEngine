#pragma once

#include "property/Base.hpp"

NS_PROPERTIES

template <typename TElement, typename TElementProperty>
struct PropertyListVector : public PropertyMinimal<std::vector<TElement>>
{
	TElementProperty elementProperty;
	
	PropertyListVector() : PropertyMinimal<std::vector<TElement>>() { this->bIsMultiline = true; }

	PropertyListVector(TElementProperty elementProp)
		: PropertyMinimal<std::vector<TElement>>()
		, elementProperty(elementProp)
	{
		this->bIsMultiline = true;
	}

	PropertyListVector(std::vector<TElement> initial, std::vector<TElement> value, TElementProperty elementProp)
		: PropertyMinimal<std::vector<TElement>>(initial, value)
		, elementProperty(elementProp)
	{
		this->bIsMultiline = true;
	}

};

bool renderList(
	const char* id, uSize size,
	std::function<void(uIndex &idx, uSize &size)> renderElement,
	std::function<void(uSize newSize)> onChangeSize
);

template <typename TElement, typename TElementProperty>
bool renderPropertyEditor(const char* id, PropertyListVector<TElement, TElementProperty> &prop)
{
	auto changeSize = [&](uSize newSize) { prop.value.resize(newSize); };
	auto renderAt = [&](uIndex &idx, uSize &size)
	{
		bool bRemoved = false;

		properties::contextMenuEntries.push_back({
			"Insert Above", [&prop, &idx, &size]()
			{
				prop.value.insert(prop.value.begin() + idx, TElement());
				idx++;
				size++;
				return true;
			}
		});
		properties::contextMenuEntries.push_back({
			"Insert Below", [&prop, &idx, &size]()
			{
				prop.value.insert(prop.value.begin() + idx + 1, TElement());
				size++;
				return true;
			}
		});
		properties::contextMenuEntries.push_back({
			"Remove", [&prop, &idx, &size, &bRemoved]()
			{
				prop.value.erase(prop.value.begin() + idx);
				size--;
				idx--;
				bRemoved = true;
				return true;
			}
		});

		prop.elementProperty.initial = idx < prop.initial.size() ? prop.initial[idx] : TElement();
		prop.elementProperty.value = prop.value[idx];
		properties::renderProperty(std::to_string(idx), prop.elementProperty);
		if (!bRemoved)
		{
			prop.value[idx] = prop.elementProperty.value;
		}

		properties::contextMenuEntries.pop_back();
		properties::contextMenuEntries.pop_back();
		properties::contextMenuEntries.pop_back();
	};
	return renderList(id, prop.value.size(), renderAt, changeSize);
}

NS_END
