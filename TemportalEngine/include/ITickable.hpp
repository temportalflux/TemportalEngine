#pragma once

#include "TemportalEnginePCH.hpp"

class ITickable : public std::enable_shared_from_this<ITickable>
{
public:
	virtual void tick(f32 deltaTime) = 0;
};
