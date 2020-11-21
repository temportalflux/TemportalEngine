#pragma once

#include "property/Base.hpp"

NS_PROPERTIES

bool renderList(
	const char* id, uSize size,
	std::function<bool(uIndex &idx, uSize &size)> renderElement,
	std::function<void(uSize newSize)> onChangeSize
);

template <typename TElement>
PropertyResult renderPropertyEditor(const char* id, std::vector<TElement> &value, std::vector<TElement> const& defaultValue)
{
	auto changeSize = [&](uSize newSize) { value.resize(newSize); };
	auto renderAt = [&](uIndex &idx, uSize &size)
	{
		bool bRemoved = false;

		properties::contextMenuEntries.push_back({
			"Insert Above", [&value, &idx, &size]()
			{
				value.insert(value.begin() + idx, TElement());
				idx++;
				size++;
				return true;
			}
		});
		properties::contextMenuEntries.push_back({
			"Insert Below", [&value, &idx, &size]()
			{
				value.insert(value.begin() + idx + 1, TElement());
				size++;
				return true;
			}
		});
		properties::contextMenuEntries.push_back({
			"Remove", [&value, &idx, &size, &bRemoved]()
			{
				value.erase(value.begin() + idx);
				size--;
				idx--;
				bRemoved = true;
				return true;
			}
		});

		TElement const defaultIdx = idx < defaultValue.size() ? defaultValue[idx] : TElement();
		TElement valueIdx = value[idx];
		bool bChangedIdx = properties::renderProperty(std::to_string(idx), valueIdx, defaultIdx);
		if (bChangedIdx && !bRemoved)
		{
			value[idx] = valueIdx;
		}

		properties::contextMenuEntries.pop_back();
		properties::contextMenuEntries.pop_back();
		properties::contextMenuEntries.pop_back();

		return bChangedIdx;
	};
	return PropertyResult::group(id, [&](bool &bChangedAny) {
		bChangedAny = renderList(id, value.size(), renderAt, changeSize);
	});
}

NS_END
