#include "physics/PhysicsObject.hpp"

#include "physics/PhysX.hpp"
#include "physics/PhysicsSystem.hpp"

using namespace physics;

void Object::setSystem(std::weak_ptr<physics::System> const& pSystem)
{
	this->mpSystem = pSystem;
}

std::shared_ptr<physics::System> Object::system() const
{
	return this->mpSystem.lock();
}
