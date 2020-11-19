#include "property/Property.hpp"

#include <imgui.h>

using namespace properties;

bool properties::renderList(
	const char* id, uSize size,
	std::function<void(uIndex &idx, uSize &size)> renderElement,
	std::function<void(uSize newSize)> onChangeSize
)
{
	bool bChangedSizeOrOrder = false;

	// List actions
	ImGui::PushID(id);
	if (ImGui::Button("Add"))
	{
		bChangedSizeOrOrder = true;
		onChangeSize(++size);
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		bChangedSizeOrOrder = true;
		size = 0;
		onChangeSize(size);
	}
	ImGui::PopID();

	// List Entries
	for (uIndex i = 0; i < size; ++i)
	{
		ImGui::PushID((i32)i);
		// NOTE: We dont actually care if the element itself changed, thats for the caller to worry about
		renderElement(i, size);
		ImGui::PopID();
	}

	return bChangedSizeOrOrder;
}

