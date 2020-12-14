#pragma once

#include "physics/PhysicsObject.hpp"

NS_PHYSICS
class System;

class Material : public Object
{
	friend class System;

public:
	Material();
	Material(Material &&other);
	Material& operator=(Material &&other);
	~Material();

	void create() override;

private:
	f32 mStaticFriction;
	f32 mDynamicFriction;
	f32 mRestitution;

	/*PxMaterial*/ void* mpInternal;

};

NS_END
