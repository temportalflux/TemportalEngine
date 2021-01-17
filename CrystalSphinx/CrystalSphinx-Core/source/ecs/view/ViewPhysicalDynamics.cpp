#include "ecs/view/ViewPhysicalDynamics.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPhysicsBody.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(PhysicalDynamics);

void PhysicalDynamics::initView(std::shared_ptr<View> pView)
{
	pView->setComponentSlots({
		ecs::component::CoordinateTransform::TypeId,
		ecs::component::PhysicsBody::TypeId
	});
}
