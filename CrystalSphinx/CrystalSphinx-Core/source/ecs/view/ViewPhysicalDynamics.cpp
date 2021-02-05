#include "ecs/view/ViewPhysicalDynamics.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPhysicsBody.hpp"

using namespace evcs;
using namespace evcs::component;
using namespace evcs::view;

DEFINE_ECS_VIEW_STATICS(PhysicalDynamics);
PhysicalDynamics::PhysicalDynamics() : View({
	CoordinateTransform::TypeId,
	PhysicsBody::TypeId
})
{
}
