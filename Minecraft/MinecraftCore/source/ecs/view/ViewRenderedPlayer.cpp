#include "ecs/view/ViewRenderedPlayer.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentCameraPOV.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(RenderedPlayer);
RenderedPlayer::RenderedPlayer() : View({
	ecs::component::CoordinateTransform::TypeId,
	ecs::component::CameraPOV::TypeId
})
{
}
