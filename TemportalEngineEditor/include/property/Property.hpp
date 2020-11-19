#pragma once

#include "TemportalEnginePCH.hpp"

#include "property/Base.hpp"
#include "property/Primitive.hpp"
#include "property/Number.hpp"
#include "property/Vector.hpp"
#include "property/Collection.hpp"

#include <imgui.h>

NS_PROPERTIES

template <typename TProperty>
bool renderContextMenu(std::string const& id, TProperty &prop, bool bOpen)
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
			prop.value = prop.initial;
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

template <typename TProperty>
bool renderProperty(std::string const id, TProperty &prop)
{
	bool bShowElement = true;
	bool bOpenContextMenu = false;
	bool bChangedValue = false;
	if (prop.bIsMultiline)
	{
		bShowElement = ImGui::TreeNode((id + "###header_" + id).c_str());
		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			bOpenContextMenu = true;
		}
	}
	if (bShowElement)
	{
		bChangedValue = properties::renderPropertyEditor(id.c_str(), prop);
		if (!prop.bIsMultiline && ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			bOpenContextMenu = true;
		}
	}
	if (prop.bIsMultiline && bShowElement)
		ImGui::TreePop();
	if (properties::renderContextMenu(id, prop, bOpenContextMenu))
	{
		bChangedValue = true;
	}
	return bChangedValue;
}

NS_END
