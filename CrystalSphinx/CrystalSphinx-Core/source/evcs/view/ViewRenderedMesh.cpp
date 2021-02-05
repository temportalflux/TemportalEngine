#include "evcs/view/ViewRenderedMesh.hpp"

#include "evcs/component/CoordinateTransform.hpp"
#include "evcs/component/ComponentRenderMesh.hpp"

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
