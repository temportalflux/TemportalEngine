#include "property/Property.hpp"

#include <imgui.h>

using namespace properties;

bool properties::renderList(
	const char* id, uSize size,
	std::function<bool(uIndex &idx, uSize &size)> renderElement,
	std::function<void(uSize newSize)> onChangeSize
)
{
	bool bChangedSizeOrderOrContent = false;

	// List actions
	ImGui::PushID(id);
	if (ImGui::Button("Add"))
	{
		bChangedSizeOrderOrContent = true;
		onChangeSize(++size);
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		bChangedSizeOrderOrContent = true;
		size = 0;
		onChangeSize(size);
	}
	ImGui::PopID();

	// List Entries
	for (uIndex i = 0; i < size; ++i)
	{
		ImGui::PushID((i32)i);
		// NOTE: We dont actually care if the element itself changed, thats for the caller to worry about
		if (renderElement(i, size)) bChangedSizeOrderOrContent = true;
		ImGui::PopID();
	}

	return bChangedSizeOrderOrContent;
}

