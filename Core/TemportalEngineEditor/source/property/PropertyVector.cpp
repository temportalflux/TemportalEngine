#include "property/PropertyVector.hpp"

#include <imgui.h>

using namespace properties;

DEFINE_PROPERTY_EDITOR(math::Color)
{
	return PropertyResult::oneLine(ImGui::ColorEdit4(id, value.data(), ImGuiColorEditFlags_None));
}
