#include "ecs/view/ViewDebugHUD.hpp"

#include "ecs/component/CoordinateTransform.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(DebugHUD);
DebugHUD::DebugHUD() : View(
	{
		ecs::component::CoordinateTransform::TypeId
	}
)
{
}
