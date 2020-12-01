#include "property/Primitive.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

using namespace properties;

PropertyResult properties::renderPropertyEditor(const char* id, bool &value, bool const& defaultValue)
{
	return PropertyResult::oneLine(ImGui::Checkbox(id, &value));
}

PropertyResult properties::renderPropertyEditor(const char* id, std::string &value, std::string const& defaultValue)
{
	return PropertyResult::oneLine(ImGui::InputText(id, &value));
}
