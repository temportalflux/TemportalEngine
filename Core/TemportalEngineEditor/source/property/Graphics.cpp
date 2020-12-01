#include "property/Property.hpp"

using namespace properties;

DEFINE_PROPERTY_EDITOR(graphics::Area)
{
	return PropertyResult::group(id, [&](bool &bChangedAny) {
		if (properties::renderProperty("Offset", value.offset, defaultValue.offset)) bChangedAny = true;
		if (properties::renderProperty("Size", value.size, defaultValue.size)) bChangedAny = true;
	});
}

DEFINE_PROPERTY_EDITOR(graphics::Viewport)
{
	return PropertyResult::group(id, [&](bool &bChangedAny) {
		if (properties::renderProperty("Offset", value.offset, defaultValue.offset)) bChangedAny = true;
		if (properties::renderProperty("Size", value.size, defaultValue.size)) bChangedAny = true;
		if (properties::renderProperty("Depth Range", value.depthRange, defaultValue.depthRange)) bChangedAny = true;
	});
}

DEFINE_PROPERTY_EDITOR(graphics::BlendMode)
{
	return PropertyResult::group(id, [&](bool &bChangedAny) {
		if (properties::renderProperty("Write Mask", value.writeMask, defaultValue.writeMask)) bChangedAny = true;
		if (properties::renderProperty("Blend", value.blend, defaultValue.blend)) bChangedAny = true;
	});
}

DEFINE_PROPERTY_EDITOR(graphics::BlendMode::Operation)
{
	return PropertyResult::group(id, [&](bool &bChangedAny) {
		if (properties::renderProperty("Color", value.color, defaultValue.color)) bChangedAny = true;
		if (properties::renderProperty("Alpha", value.alpha, defaultValue.alpha)) bChangedAny = true;
	});
}

DEFINE_PROPERTY_EDITOR(graphics::BlendMode::Component)
{
	PropertyResult result;
	result.bChangedValue = false;
	result.bIsHovered = false;

	ImGui::PushID(id);
	{
		// Label
		{
			// TODO: Might be able to use a group for 1 hovered check
			ImGui::Text(id);
			if (ImGui::IsItemHovered()) result.bIsHovered = true;
			ImGui::SameLine();
			ImGui::Text(" = ");
			if (ImGui::IsItemHovered()) result.bIsHovered = true;
		}

		ImGui::SameLine();

		// Src
		{
			ImGui::PushItemWidth(150);
			if (properties::renderProperty("###src", value.srcFactor, defaultValue.srcFactor))
				result.bChangedValue = true;
			ImGui::PopItemWidth();
		}

		ImGui::SameLine();

		// Operation
		{
			ImGui::PushItemWidth(50);
			if (properties::renderProperty("###op", value.operation, defaultValue.operation))
				result.bChangedValue = true;
			ImGui::PopItemWidth();
		}

		ImGui::SameLine();

		// Dst
		{
			ImGui::PushItemWidth(150);
			if (properties::renderProperty("###dst", value.dstFactor, defaultValue.dstFactor))
				result.bChangedValue = true;
			ImGui::PopItemWidth();
		}

	}
	ImGui::PopID();

	return result;
}

DEFINE_PROPERTY_EDITOR(graphics::PhysicalDevicePreference)
{
	return PropertyResult::group(id, [&](bool &bChangedAny) {
		if (properties::renderProperty("Type", value.getDeviceTypes(), defaultValue.getDeviceTypes())) bChangedAny = true;
		if (properties::renderProperty("Extensions", value.getDeviceExtensions(), defaultValue.getDeviceExtensions())) bChangedAny = true;
		if (properties::renderProperty("Features", value.getFeatures(), defaultValue.getFeatures())) bChangedAny = true;
		if (properties::renderProperty("Queue Families", value.getQueueFamilies(), defaultValue.getQueueFamilies())) bChangedAny = true;
		if (properties::renderProperty("Swap Chain Support", value.getSwapChain(), defaultValue.getSwapChain())) bChangedAny = true;
	});
}

bool properties::renderPreferenceScore(std::optional<ui8> &value)
{
	bool bChangedAny = false;
	bool bIsRequired = !value.has_value();
	if (ImGui::Checkbox("Required", &bIsRequired))
	{
		bChangedAny = true;
		if (bIsRequired) value = std::nullopt;
		else value = 0;
	}
	if (value)
	{
		ImGui::SameLine();
		ImGui::PushItemWidth(75);
		ui8 defaultScore = 0;
		if (properties::renderNumbers<ui8>("Score", &defaultScore, &value.value(), 1).bChangedValue) bChangedAny = true;
		ImGui::PopItemWidth();
	}
	return bChangedAny;
}
