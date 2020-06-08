#pragma once

#include "TemportalEnginePCH.hpp"

NS_GUI

template <typename T>
class Optional
{

public:

	static bool Inline(std::string titleId, std::optional<T> &item, std::string toggleTitleId, bool bHasValueWhenChecked, std::function<bool(T &value)> renderItem)
	{
		bool bChanged = false;
		bool bToggledOn = item.has_value() == bHasValueWhenChecked;
		if (ImGui::Checkbox(toggleTitleId.c_str(), &bToggledOn))
		{
			item = bToggledOn == bHasValueWhenChecked ? std::make_optional(T()) : std::nullopt;
			bChanged = true;
		}
		if (item.has_value())
		{
			T v = item.value();
			if (renderItem(v))
			{
				item = v;
				bChanged = true;
			}
		}
		return bChanged;
	}

};

NS_END
