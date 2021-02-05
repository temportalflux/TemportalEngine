#include "evcs/view/ViewPlayerCamera.hpp"

#include "evcs/component/CoordinateTransform.hpp"
#include "evcs/component/ComponentCameraPOV.hpp"
#include "evcs/component/ComponentRenderMesh.hpp"

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
