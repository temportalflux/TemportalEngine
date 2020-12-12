#include "ecs/view/ViewPlayerCamera.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentCameraPOV.hpp"
#include "ecs/component/ComponentRenderMesh.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(PlayerCamera);
PlayerCamera::PlayerCamera() : View({
	ecs::component::CoordinateTransform::TypeId,
	ecs::component::CameraPOV::TypeId,
	ecs::component::RenderMesh::TypeId,
})
{
}
