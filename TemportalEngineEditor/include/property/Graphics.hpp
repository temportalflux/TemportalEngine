#pragma once

#include "property/Base.hpp"

#include "graphics/Area.hpp"
#include "graphics/BlendMode.hpp"
#include "graphics/types.hpp"

NS_PROPERTIES

DECLARE_PROPERTY_EDITOR(graphics::Area);
DECLARE_PROPERTY_EDITOR(graphics::Viewport);
DECLARE_PROPERTY_EDITOR(graphics::BlendMode);
DECLARE_PROPERTY_EDITOR(graphics::BlendMode::Operation);
DECLARE_PROPERTY_EDITOR(graphics::BlendMode::Component);

NS_END
