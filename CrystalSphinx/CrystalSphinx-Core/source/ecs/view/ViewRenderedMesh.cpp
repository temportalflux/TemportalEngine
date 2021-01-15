#include "ecs/view/ViewRenderedMesh.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentRenderMesh.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(RenderedMesh);
RenderedMesh::RenderedMesh() : View({
	ecs::component::CoordinateTransform::TypeId,
	ecs::component::RenderMesh::TypeId
})
{
}
