#include "property/Property.hpp"

#include <imgui.h>

using namespace properties;

DEFINE_PROPERTY_EDITOR(Version)
{
	PropertyResult result;
	result.bChangedValue = false;

	#define VERSION_MEMBER_PROP(LABEL, UNPACKED_COMP) \
		{ \
			auto verValue = value.unpacked.UNPACKED_COMP; \
			if (properties::renderProperty(LABEL, verValue, defaultValue.unpacked.UNPACKED_COMP)) \
			{ \
				value.unpacked.UNPACKED_COMP = verValue; \
				result.bChangedValue = true; \
			} \
		}
	
	ImGui::Text(id);
	result.bIsHovered = ImGui::IsItemHovered();
	ImGui::SameLine();

	ImGui::PushItemWidth(50);

	VERSION_MEMBER_PROP("Major", major)
	ImGui::SameLine();
	VERSION_MEMBER_PROP("Minor", minor)
	ImGui::SameLine();
	VERSION_MEMBER_PROP("Patch", patch)

	ImGui::PopItemWidth();

	#undef VERSION_MEMBER_PROP

	return result;
}
