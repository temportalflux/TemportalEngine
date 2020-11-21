#include "property/Property.hpp"

#include <imgui.h>

using namespace properties;

DEFINE_PROPERTY_EDITOR(asset::Pipeline::DescriptorGroup)
{
	return properties::renderPropertyEditor(id, value.descriptors, defaultValue.descriptors);
}

DEFINE_PROPERTY_EDITOR(asset::Pipeline::Descriptor)
{
	PropertyResult result;
	result.bChangedValue = false;
	result.bIsHovered = false;
	
	ImGui::Text(id);
	if (ImGui::IsItemHovered()) result.bIsHovered = true;
	ImGui::SameLine();
	ImGui::Text(": ");
	if (ImGui::IsItemHovered()) result.bIsHovered = true;

	ImGui::SameLine();

	ImGui::PushItemWidth(100);
	if (properties::renderProperty("Identifier", value.id, defaultValue.id)) result.bChangedValue = true;
	ImGui::PopItemWidth();

	ImGui::SameLine();

	ImGui::PushItemWidth(200);
	if (properties::renderProperty("Type", value.type, defaultValue.type)) result.bChangedValue = true;
	ImGui::PopItemWidth();

	ImGui::SameLine();

	ImGui::PushItemWidth(200);
	if (properties::renderProperty("Stage", value.stage, defaultValue.stage)) result.bChangedValue = true;
	ImGui::PopItemWidth();

	return result;
}
