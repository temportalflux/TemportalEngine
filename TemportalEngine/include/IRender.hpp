#pragma once

#include "TemportalEnginePCH.hpp"

#include "WorldObject.hpp"

FORWARD_DEF(NS_GRAPHICS, class Command)

class IRender
{
public:
	// TODO: pass in the objects being rendered
	virtual void draw(graphics::Command *command) = 0;
};
