#pragma once

#include "TemportalEnginePCH.hpp"

#include "property/Base.hpp"
#include "property/Primitive.hpp"
#include "property/Number.hpp"
#include "property/PropertyVector.hpp"
#include "property/Collection.hpp"
#include "property/Graphics.hpp"
#include "property/Utility.hpp"
#include "property/Asset.hpp"
#include "property/asset/PropertyGraphicsAsset.hpp"

#include <imgui.h>

NS_PROPERTIES

template <typename TValue>
bool renderContextMenu(std::string const& id, TValue &value, TValue const& defaultValue, bool bOpen)
{
	auto const popupId = id + "_context";
	if (bOpen)
	{
		ImGui::OpenPopup(popupId.c_str());
	}
	bool bChangedValue = false;
	if (ImGui::BeginPopup(popupId.c_str()))
	{
		if (ImGui::Selectable("Copy"))
		{
			ImGui::SetClipboardText(("TODO Copy " + id).c_str());
		}
		if (ImGui::Selectable("Paste"))
		{
			// TODO: Paste a copied thing
		}
		if (ImGui::Selectable("Reset to Default"))
		{
			value = defaultValue;
			bChangedValue = true;
		}
		for (auto const& entry : properties::contextMenuEntries)
		{
			if (ImGui::Selectable(entry.label.c_str()) && entry.action())
			{
				bChangedValue = true;
			}
		}
		ImGui::EndPopup();
	}
	return bChangedValue;
}

template <typename TValue>
bool renderProperty_internal(std::string id, TValue &value, TValue const& defaultValue)
{
	PropertyResult result = properties::renderPropertyEditor(id.c_str(), value, defaultValue);
	bool bOpenContextMenu = result.bIsHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right);
	if (properties::renderContextMenu(id, value, defaultValue, bOpenContextMenu))
	{
		result.bChangedValue = true;
	}
	return result.bChangedValue;
}

template <typename TValue>
bool renderProperty(std::string id, TValue &value, TValue const& defaultValue)
{
	return properties::renderProperty_internal<TValue>(id, value, defaultValue);
}

NS_END
