#include "ecs/view/ViewCameraPerspective.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentCameraPOV.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(CameraPerspective);
CameraPerspective::CameraPerspective() : View({
	ecs::component::CoordinateTransform::TypeId,
	ecs::component::CameraPOV::TypeId
})
{
}
