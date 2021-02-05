#include "ecs/view/ViewRenderedMesh.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentRenderMesh.hpp"

using namespace evcs;
using namespace evcs::component;
using namespace evcs::view;

DEFINE_ECS_VIEW_STATICS(RenderedMesh);
RenderedMesh::RenderedMesh() : View({
	CoordinateTransform::TypeId,
	RenderMesh::TypeId
})
{
}
