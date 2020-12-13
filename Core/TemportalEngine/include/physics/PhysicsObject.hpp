#pragma once

#include "TemportalEnginePCH.hpp"

NS_PHYSICS
class System;

class Object
{

public:
	void setSystem(std::weak_ptr<physics::System> const& pSystem);
	std::shared_ptr<physics::System> system() const;
	virtual void create() = 0;

private:
	std::weak_ptr<physics::System> mpSystem;

};

NS_END
