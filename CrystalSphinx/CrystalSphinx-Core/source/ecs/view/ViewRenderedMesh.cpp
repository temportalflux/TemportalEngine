#include "ecs/view/ViewRenderedMesh.hpp"

#include "ecs/component/CoordinateTransform.hpp"
#include "ecs/component/ComponentRenderMesh.hpp"

using namespace ecs;
using namespace ecs::view;

DEFINE_ECS_VIEW_STATICS(RenderedMesh);

void RenderedMesh::initView(std::shared_ptr<View> pView)
{
	pView->setComponentSlots({
		ecs::component::CoordinateTransform::TypeId,
		ecs::component::RenderMesh::TypeId
	});
}
