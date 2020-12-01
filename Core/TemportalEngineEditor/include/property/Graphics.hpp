#pragma once

#include "property/Base.hpp"

#include "graphics/Area.hpp"
#include "graphics/BlendMode.hpp"
#include "graphics/types.hpp"
#include "graphics/PhysicalDevicePreference.hpp"
#include "graphics/RenderPassMeta.hpp"

#include <imgui.h>

NS_PROPERTIES

DECLARE_PROPERTY_EDITOR(graphics::Area);
DECLARE_PROPERTY_EDITOR(graphics::Viewport);
DECLARE_PROPERTY_EDITOR(graphics::BlendMode);
DECLARE_PROPERTY_EDITOR(graphics::BlendMode::Operation);
DECLARE_PROPERTY_EDITOR(graphics::BlendMode::Component);

DECLARE_PROPERTY_EDITOR(graphics::PhysicalDevicePreference);

bool renderPreferenceScore(std::optional<ui8> &value);

template <typename T>
DECLARE_PROPERTY_EDITOR(graphics::PhysicalDevicePreference::Preference<T>)
{
	return PropertyResult::group(id, [&](bool &bChangedAny) {
		if (renderPreferenceScore(value.score)) bChangedAny = true;
		ImGui::SameLine();
		ImGui::PushItemWidth(200);
		if (properties::renderProperty("##value", value.value, defaultValue.value)) bChangedAny = true;
		ImGui::PopItemWidth();
	});
}

NS_END
