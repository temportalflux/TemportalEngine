#include "ecs/system/SystemPhysicsIntegration.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"
#include "ecs/view/ViewPhysicalDynamics.hpp"
#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPhysicsBody.hpp"

using namespace ecs;
using namespace ecs::system;

PhysicsIntegration::PhysicsIntegration() : System(view::PhysicalDynamics::TypeId)
{
}

void PhysicsIntegration::update(f32 deltaTime, std::shared_ptr<view::View> view)
{
	OPTICK_EVENT();
	static logging::Logger ControllerLog = DeclareLog("Physics");

	auto transform = view->get<component::CoordinateTransform>();
	auto body = view->get<component::PhysicsBody>();
	assert(transform && body);

	transform->position() += body->integrateLinear(deltaTime);

}
