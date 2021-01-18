#include "ecs/component/ComponentPlayerPhysics.hpp"

#include "game/GameInstance.hpp"
#include "game/GameClient.hpp"
#include "world/World.hpp"
#include "network/NetworkInterface.hpp"

using namespace ecs;
using namespace ecs::component;

DEFINE_ECS_COMPONENT_STATICS(PlayerPhysics)

PlayerPhysics::PlayerPhysics()
	: Component()
	, mbAffectedByGravity(true)
	, mCollisionExtents({ 0.4f, 0.9f, 0.4f })
{
}

std::vector<Component::Field> PlayerPhysics::allFields() const
{
	return {
		ECS_FIELD(PlayerPhysics, mbAffectedByGravity),
		ECS_FIELD(PlayerPhysics, mCollisionExtents)
	};
}

PlayerPhysics::~PlayerPhysics()
{
}

PlayerPhysics& PlayerPhysics::setIsAffectedByGravity(bool bAffectedByGravity)
{
	this->mbAffectedByGravity = bAffectedByGravity;
	return *this;
}

bool PlayerPhysics::isAffectedByGravity() const { return this->mbAffectedByGravity; }

math::Vector3 const& PlayerPhysics::collisionExtents() const { return this->mCollisionExtents; }

void PlayerPhysics::validate()
{
	// NOTE: can validate `mbIsAffectedByGravity` here
}
