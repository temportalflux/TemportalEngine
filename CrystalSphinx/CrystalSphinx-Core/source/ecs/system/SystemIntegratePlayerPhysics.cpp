#include "ecs/system/SystemIntegratePlayerPhysics.hpp"

#include "Engine.hpp"
#include "ecs/view/ViewPlayerPhysics.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerPhysics.hpp"
#include "logging/Logger.hpp"
#include "physics/PhysicsScene.hpp"
#include "physics/PhysicsController.hpp"

#include "game/GameInstance.hpp"
#include "world/World.hpp"

using namespace ecs;
using namespace ecs::system;

IntegratePlayerPhysics::IntegratePlayerPhysics() : System(view::PlayerPhysics::TypeId)
{
}

void IntegratePlayerPhysics::tick(f32 deltaTime)
{
	auto& ecs = engine::Engine::Get()->getECS();
	if (game::Game::networkInterface()->type().includes(network::EType::eServer))
	{
		ecs.beginReplication();
	}
	
	System::tick(deltaTime);

	ecs.endReplication();
}

void IntegratePlayerPhysics::update(f32 deltaTime, view::View* view)
{
	OPTICK_EVENT();

	auto pWorld = game::Game::Get()->world();
	assert(pWorld);

	auto transform = view->get<component::CoordinateTransform>();
	auto physicsComp = view->get<component::PlayerPhysics>();
	assert(transform && physicsComp);
	assert(physicsComp->owner()); // assumes that transform and physics have the same owner

	auto ownerNetId = physicsComp->owner().value();
	if (!pWorld->hasPhysicsController(ownerNetId)) return;
	auto& controller = pWorld->getPhysicsController(ownerNetId);

	auto displacement = math::Vector3::ZERO;
	if (physicsComp->isAffectedByGravity())
	{
		displacement += controller.scene()->gravity() * deltaTime;
	}

	controller.move(displacement, deltaTime);
	transform->setPosition(world::Coordinate::fromGlobal(controller.footPosition()));
}
