#include "ecs/system/SystemPhysicsIntegration.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"
#include "ecs/view/ViewPhysicsBody.hpp"
#include "ecs/component/CoordinateTransform.hpp"

using namespace ecs;
using namespace ecs::system;

PhysicsIntegration::PhysicsIntegration() : System(view::PhysicsBody::TypeId)
{
}

void PhysicsIntegration::update(f32 deltaTime, std::shared_ptr<view::View> view)
{
	OPTICK_EVENT();
	static logging::Logger ControllerLog = DeclareLog("Physics");

	auto transform = view->get<component::CoordinateTransform>();
	assert(transform);

	transform->position() += transform->linearVelocity() * deltaTime;
	transform->position() += 0.5f * transform->linearAccelleration() * deltaTime * deltaTime;
	transform->linearVelocity() += transform->linearAccelleration() * deltaTime;

}
