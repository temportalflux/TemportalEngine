#include "physics/PhysicsRigidBody.hpp"

#include "physics/PhysicsSystem.hpp"
#include "physics/PhysicsShape.hpp"
#include "physics/PhysX.hpp"
#include "utility/Casting.hpp"

using namespace physics;

RigidBody::RigidBody() : mbIsStatic(false), mpInternal(nullptr)
{
}

RigidBody::RigidBody(RigidBody &&other) { *this = std::move(other); }

RigidBody& RigidBody::operator=(RigidBody &&other)
{
	this->mpSystem = other.mpSystem;
	other.mpSystem.reset();

	this->mInitialPosition = other.mInitialPosition;
	this->mInitialRotation = other.mInitialRotation;
	this->mbIsStatic = other.mbIsStatic;

	this->mpInternal = other.mpInternal;
	other.mpInternal = nullptr;

	return *this;
}

RigidBody::~RigidBody()
{
	release();
}

void RigidBody::release()
{
	if (this->mpInternal != nullptr)
	{
		assert(hasSystem());
		as<physx::PxRigidBody>(this->mpInternal)->release();
		this->mpInternal = nullptr;
	}
}

RigidBody& RigidBody::setInitialTransform(math::Vector3 const& position, math::Quaternion const& rotation)
{
	assert(this->mpInternal == nullptr);
	this->mInitialPosition = position;
	this->mInitialRotation = rotation;
	return *this;
}

RigidBody& RigidBody::setIsStatic(bool bIsStatic)
{
	assert(this->mpInternal == nullptr);
	this->mbIsStatic = bIsStatic;
	return *this;
}

bool RigidBody::isStatic() const { return this->mbIsStatic; }

void RigidBody::create()
{
	assert(hasSystem());
	assert(this->mpInternal == nullptr);
	this->mpInternal = this->system()->createRigidBody(this);
}

void RigidBody::createPlane(Shape *pShape)
{
	assert(hasSystem());
	this->mpInternal = this->system()->createRigidBodyPlane(this, pShape);
}

void RigidBody::createPlane(math::Vector3 const& normal, f32 const& distance, Material *pMaterial)
{
	assert(hasSystem());
	auto& shape = Shape().setMaterial(pMaterial).setAsPlane(normal, distance);
	this->createPlane(&shape);
}

RigidBody& RigidBody::attachShape(Shape *pShape)
{
	assert(this->mpInternal != nullptr);
	assert(pShape != nullptr && pShape->isValid());
	// planes can only be applied to static rigid bodies
	assert(pShape->type() != EShapeType::ePlane || this->isStatic());
	as<physx::PxRigidBody>(this->mpInternal)->attachShape(*extract<physx::PxShape>(pShape));
	return *this;
}

RigidBody& RigidBody::setLinearVelocity(math::Vector3 const& v)
{
	assert(this->mpInternal != nullptr);
	as<physx::PxRigidBody>(this->mpInternal)->setLinearVelocity(
		physics::toPhysX(v)
	);
	return *this;
}
