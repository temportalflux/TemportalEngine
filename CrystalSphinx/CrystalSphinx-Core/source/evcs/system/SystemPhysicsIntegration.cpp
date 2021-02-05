#include "evcs/system/SystemPhysicsIntegration.hpp"

#include "Engine.hpp"
#include "logging/Logger.hpp"
#include "evcs/view/ViewPhysicalDynamics.hpp"
#include "evcs/component/CoordinateTransform.hpp"
#include "evcs/component/ComponentPhysicsBody.hpp"

using namespace evcs;
using namespace evcs::system;

PhysicsIntegration::PhysicsIntegration() : System(view::PhysicalDynamics::TypeId)
{
}

void PhysicsIntegration::update(f32 deltaTime, view::View* view)
{
	OPTICK_EVENT();
	static logging::Logger ControllerLog = DeclareLog("Physics", LOG_INFO);

	auto transform = view->get<component::CoordinateTransform>();
	auto body = view->get<component::PhysicsBody>();
	assert(transform && body);

	transform->position() += body->integrateLinear(deltaTime);

}
