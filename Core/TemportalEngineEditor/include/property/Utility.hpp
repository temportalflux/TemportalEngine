#pragma once

#include "property/Base.hpp"

#include "utility/Flags.hpp"
#include "utility/Version.hpp"

NS_PROPERTIES

template <typename TEnum>
DECLARE_PROPERTY_EDITOR(utility::EnumWrapper<TEnum>)
{
	PropertyResult result;
	result.bChangedValue = false;
	bool bShowOptions = ImGui::BeginCombo(id, value.to_display_string().c_str(), ImGuiComboFlags_None);
	result.bIsHovered = ImGui::IsItemHovered();
	if (bShowOptions)
	{
		for (const auto& option : utility::EnumWrapper<TEnum>::ALL)
		{
			bool bIsSelected = option == value;
			ImGui::PushID((ui64)option);
			if (ImGui::Selectable(utility::EnumWrapper<TEnum>(option).to_display_string().c_str(), bIsSelected))
			{
				value = option;
				result.bChangedValue = true;
			}
			if (bIsSelected) ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	return result;
}

template <typename TEnum>
DECLARE_PROPERTY_EDITOR(utility::Flags<TEnum>)
{
	PropertyResult result;
	result.bChangedValue = false;
	auto const& selected = value.toSet();
	bool bShowOptions = ImGui::BeginCombo(id, value.to_string().c_str(), ImGuiComboFlags_None);
	result.bIsHovered = ImGui::IsItemHovered();
	if (bShowOptions)
	{
		for (const auto& option : value.all())
		{
			bool bIsSelected = selected.find(option) != selected.end();
			ImGui::PushID((ui32)option);
			if (ImGui::Selectable(utility::EnumWrapper<TEnum>(option).to_display_string().c_str(), bIsSelected))
			{
				if (!bIsSelected) value |= option;
				else value ^= option;
				result.bChangedValue = true;
			}
			if (bIsSelected) ImGui::SetItemDefaultFocus();
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	return result;
}

template <typename T>
DECLARE_PROPERTY_EDITOR(std::optional<T>)
{
	PropertyResult result;
	result.bChangedValue = false;
	bool bShowContent = ImGui::TreeNode(id);
	result.bIsHovered = ImGui::IsItemHovered();
	if (bShowContent)
	{
		ImGui::PushID(id);
		bool bToggledOn = value.has_value();
		if (ImGui::Checkbox(id, &bToggledOn) && bToggledOn != value.has_value())
		{
			value = bToggledOn ? std::make_optional(T()) : std::nullopt;
			result.bChangedValue = true;
		}
		if (value.has_value())
		{
			if (renderProperty("Value", value.value(), defaultValue ? *defaultValue : T()))
			{
				result.bChangedValue = true;
			}
		}
		ImGui::PopID();
		ImGui::TreePop();
	}
	return result;
}

DECLARE_PROPERTY_EDITOR(Version);

NS_END
