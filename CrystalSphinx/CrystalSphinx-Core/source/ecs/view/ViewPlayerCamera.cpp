#include "ecs/view/ViewPlayerCamera.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentCameraPOV.hpp"
#include "ecs/component/ComponentRenderMesh.hpp"

using namespace evcs;
using namespace evcs::component;
using namespace evcs::view;

DEFINE_ECS_VIEW_STATICS(PlayerCamera);
PlayerCamera::PlayerCamera() : View({
	CoordinateTransform::TypeId,
	CameraPOV::TypeId,
	RenderMesh::TypeId,
})
{
}
