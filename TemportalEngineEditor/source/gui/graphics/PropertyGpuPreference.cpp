#include "gui/graphics/PropertyGpuPreference.hpp"

#include "gui/widget/List.hpp"
#include "gui/widget/Optional.hpp"
#include "gui/widget/FieldNumber.hpp"
#include "gui/widget/Combo.hpp"

#include <imgui.h>

using namespace gui;

PropertyGpuPreference::PropertyGpuPreference(Internal const &value)
	: mInternal(value)
{
}

bool PropertyGpuPreference::render(char const* titleId)
{
	bool bHasChanged = false;

	if (gui::List<Preference<EDeviceType::Enum>>::Inline(
		"type", "Type", /*values collapse*/ false,
		this->mInternal.getDeviceTypes(), &PropertyGpuPreference::renderPrefDeviceType
	)) bHasChanged = true;

	if (gui::List<Preference<EExtension::Type>>::Inline(
		"ext", "Extensions", /*values collapse*/ false,
		this->mInternal.getDeviceExtensions(), &PropertyGpuPreference::renderPrefExtension
	)) bHasChanged = true;

	if (gui::List<Preference<EFeature::Enum>>::Inline(
		"feature", "Features", /*values collapse*/ false,
		this->mInternal.getFeatures(), &PropertyGpuPreference::renderPrefFeature
	)) bHasChanged = true;

	if (gui::List<Preference<EQueueFamily::Enum>>::Inline(
		"queues", "Queue Families", /*values collapse*/ false,
		this->mInternal.getQueueFamilies(), &PropertyGpuPreference::renderPrefQueueFam
	)) bHasChanged = true;

	if (gui::List<Preference<ESwapChain::Enum>>::Inline(
		"swapchain", "Swap Chain Support", /*values collapse*/ false,
		this->mInternal.getSwapChain(), &PropertyGpuPreference::renderPrefSwapChain
	)) bHasChanged = true;

	return bHasChanged;
}

bool renderOptionalScore(std::optional<ui8> &value)
{
	return gui::Optional<ui8>::Inline(value, "Required", false, [](ui8 &score)
	{
		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		auto bChanged = gui::FieldNumber<ui8, 1>::InlineSingle("##score", score);
		ImGui::PopItemWidth();
		return bChanged;
	});
}

bool PropertyGpuPreference::renderPrefDeviceType(uIndex const& idx, Preference<EDeviceType::Enum>& value)
{
	bool bHasChanged = renderOptionalScore(value.score);
	ImGui::SameLine();

	// Dropdown for device type
	ImGui::PushItemWidth(200);
	if (gui::Combo<EDeviceType::Enum>::Inline(
		"##value", EDeviceType::ALL, value.value,
		EDeviceType::to_string,
		[](EDeviceType::Enum type) { ImGui::PushID((ui32)type); }
	)) bHasChanged = true;
	ImGui::PopItemWidth();

	return bHasChanged;
}

bool PropertyGpuPreference::renderPrefExtension(uIndex const& idx, Preference<EExtension::Type>& value)
{
	bool bHasChanged = renderOptionalScore(value.score);
	ImGui::SameLine();

	// Dropdown for extensions
	ImGui::PushItemWidth(200);
	if (gui::Combo<EExtension::Type>::Inline(
		"##value", EExtension::ALL, value.value,
		/*to str*/ [](std::string s) { return s; },
		/*push id*/ [](std::string s) { ImGui::PushID(s.c_str()); }
	)) bHasChanged = true;
	ImGui::PopItemWidth();

	return bHasChanged;
}

bool PropertyGpuPreference::renderPrefFeature(uIndex const& idx, Preference<EFeature::Enum>& value)
{
	bool bHasChanged = renderOptionalScore(value.score);
	ImGui::SameLine();

	// Dropdown for extensions
	ImGui::PushItemWidth(200);
	if (gui::Combo<EFeature::Enum>::Inline(
		"##value", EFeature::ALL, value.value,
		EFeature::to_string,
		/*push id*/ [](EFeature::Enum e) { ImGui::PushID((ui32)e); }
	)) bHasChanged = true;
	ImGui::PopItemWidth();

	return bHasChanged;
}

bool PropertyGpuPreference::renderPrefQueueFam(uIndex const& idx, Preference<EQueueFamily::Enum> &value)
{
	bool bHasChanged = renderOptionalScore(value.score);
	ImGui::SameLine();

	// Dropdown for device type
	ImGui::PushItemWidth(200);
	if (gui::Combo<EQueueFamily::Enum>::Inline(
		"##value", EQueueFamily::ALL, value.value,
		EQueueFamily::to_string,
		[](EQueueFamily::Enum type) { ImGui::PushID((ui32)type); }
	)) bHasChanged = true;
	ImGui::PopItemWidth();

	return bHasChanged;
}

bool PropertyGpuPreference::renderPrefSwapChain(uIndex const& idx, Preference<ESwapChain::Enum> &value)
{
	bool bHasChanged = renderOptionalScore(value.score);
	ImGui::SameLine();

	// Dropdown for device type
	ImGui::PushItemWidth(200);
	if (gui::Combo<ESwapChain::Enum>::Inline(
		"##value", ESwapChain::ALL, value.value,
		ESwapChain::to_string,
		[](ESwapChain::Enum type) { ImGui::PushID((ui32)type); }
	)) bHasChanged = true;
	ImGui::PopItemWidth();

	return bHasChanged;
}
