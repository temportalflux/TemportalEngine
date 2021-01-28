#pragma once

#include "TemportalEnginePCH.hpp"

class ITickable : public virtual_enable_shared_from_this<ITickable>
{
public:
	virtual void tick(f32 deltaTime) = 0;
};
