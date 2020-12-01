#pragma once

#include "TemportalEnginePCH.hpp"

#include "WorldObject.hpp"

FORWARD_DEF(NS_GRAPHICS, class Command)

class IRender
{
public:
	virtual bool reRecordRequired() { return false; }
	// TODO: pass in the objects being rendered
	virtual void record(graphics::Command *command) = 0;
};
