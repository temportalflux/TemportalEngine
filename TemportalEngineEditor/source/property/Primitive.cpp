#include "property/Primitive.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

using namespace properties;

bool properties::renderPropertyEditor(const char* id, PropertyMinimal<bool> &prop)
{
	return ImGui::Checkbox(id, &prop.value);
}

bool properties::renderPropertyEditor(const char* id, PropertyMinimal<std::string> &prop)
{
	return ImGui::InputText(id, &prop.value);
}
