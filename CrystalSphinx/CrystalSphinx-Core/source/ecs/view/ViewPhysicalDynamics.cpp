#include "ecs/view/ViewPhysicalDynamics.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPhysicsBody.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(PhysicalDynamics);
PhysicalDynamics::PhysicalDynamics() : View({
	ecs::component::CoordinateTransform::TypeId,
	ecs::component::PhysicsBody::TypeId
})
{
}
