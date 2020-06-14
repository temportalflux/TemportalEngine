#pragma once

#include "TemportalEnginePCH.hpp"

#include "graphics/AttributeBinding.hpp"
#include "WorldObject.hpp"

FORWARD_DEF(NS_GRAPHICS, class GameRenderer)

// System for rendering a set of cubes
class RenderCube
{

public:
	void appendBindings(std::vector<graphics::AttributeBinding> &bindings, ui8 &slot) const;
	// TODO: Should take an array of a custom structure which maps to specific components (transform)
	void init(graphics::GameRenderer *renderer, std::vector<WorldObject> const &instances);
	void draw(graphics::GameRenderer *renderer, std::vector<WorldObject> const &instances);

};
