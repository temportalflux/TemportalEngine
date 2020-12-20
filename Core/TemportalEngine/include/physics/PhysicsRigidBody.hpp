#pragma once

#include "physics/PhysicsObject.hpp"

NS_PHYSICS
class Material;
class Shape;

class RigidBody : public Object
{
	friend class System;

public:
	RigidBody();
	~RigidBody();
	void release();

	// TODO: Figure out some way to add a reference frame to physx so that demo/MC chunk coordinates arent combined with the local position
	RigidBody& setInitialTransform(math::Vector3 const& position, math::Quaternion const& rotation);
	RigidBody& setIsStatic(bool bIsStatic);
	bool isStatic() const;

	void create() override;
	void createPlane(Shape *pShape);
	void createPlane(math::Vector3 const& normal, f32 const& distance, Material *pMaterial);

	bool hasValue() const { return this->mpInternal != nullptr; }
	operator bool() const { return hasValue(); }
	void* get() const { return this->mpInternal; }

	RigidBody& attachShape(Shape *pShape);
	RigidBody& setLinearVelocity(math::Vector3 const& v);

private:
	math::Vector3 mInitialPosition;
	math::Quaternion mInitialRotation;
	bool mbIsStatic;

	/*PxRigidBody*/ void* mpInternal;

};

NS_END
