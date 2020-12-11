#include "ecs/view/ViewPhysicsBody.hpp"

#include "ecs/component/CoordinateTransform.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(PhysicsBody);
PhysicsBody::PhysicsBody() : View({
	ecs::component::CoordinateTransform::TypeId
})
{
}
