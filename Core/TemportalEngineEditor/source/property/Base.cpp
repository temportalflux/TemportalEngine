#include "property/Base.hpp"

#include <imgui.h>

using namespace properties;

PropertyResult PropertyResult::oneLine(bool bChangedValue)
{
	PropertyResult result;
	result.bChangedValue = bChangedValue;
	result.bIsHovered = ImGui::IsItemHovered();
	return result;
}

PropertyResult PropertyResult::group(char const* id, std::function<void(bool &bChangedAny)> renderContent)
{
	PropertyResult result;
	result.bChangedValue = false;
	bool bShowContent = ImGui::TreeNode(id);
	result.bIsHovered = ImGui::IsItemHovered();
	if (bShowContent)
	{
		ImGui::PushID(id);
		renderContent(result.bChangedValue);
		ImGui::PopID();
		ImGui::TreePop();
	}
	return result;
}
