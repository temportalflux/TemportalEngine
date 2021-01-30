#include "ecs/view/ViewPlayerPhysics.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentPlayerPhysics.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(view::PlayerPhysics);
view::PlayerPhysics::PlayerPhysics() : View({
	ecs::component::CoordinateTransform::TypeId,
	ecs::component::PlayerPhysics::TypeId
})
{
}
