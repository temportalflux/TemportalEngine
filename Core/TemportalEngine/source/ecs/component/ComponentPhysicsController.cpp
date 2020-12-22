#include "ecs/component/ComponentPhysicsController.hpp"

#include "physics/PhysicsScene.hpp"

using namespace ecs;
using namespace ecs::component;

DEFINE_ECS_COMPONENT_STATICS(PhysicsController)

PhysicsController::PhysicsController()
	: Component()
	, mbAffectedByGravity(true)
{
}

PhysicsController::~PhysicsController()
{
	this->mController.release();
}

physics::Controller& PhysicsController::controller()
{
	return this->mController;
}

PhysicsController& PhysicsController::setIsAffectedByGravity(bool bAffectedByGravity)
{
	this->mbAffectedByGravity = bAffectedByGravity;
	return *this;
}

bool PhysicsController::isAffectedByGravity() const { return this->mbAffectedByGravity; }
