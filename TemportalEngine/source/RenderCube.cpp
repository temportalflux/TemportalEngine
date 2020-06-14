#include "RenderCube.hpp"

#include "graphics/GameRenderer.hpp"
#include "ModelCube.hpp"

void RenderCube::appendBindings(std::vector<graphics::AttributeBinding> &bindings, ui8 &slot) const
{
	auto additionalBindings = ModelCube::bindings(slot);
	bindings.insert(
		std::end(bindings),
		std::begin(additionalBindings),
		std::end(additionalBindings)
	);
}

void RenderCube::init(graphics::GameRenderer *renderer, std::vector<WorldObject> const &instances)
{
	auto model = ModelCube();
	renderer->createInputBuffers(
		model.getVertexBufferSize(),
		model.getIndexBufferSize(),
		(ui32)instances.size() * sizeof(WorldObject::InstanceData)
	);
	
	renderer->writeVertexData(0, model.verticies());
	renderer->writeIndexData(0, model.indicies());

	auto instanceData = std::vector<WorldObject::InstanceData>(instances.size());
	std::transform(
		std::begin(instances), std::end(instances),
		std::begin(instanceData),
		[](WorldObject inst) -> WorldObject::InstanceData { return { inst.getModelMatrix() }; }
	);
	renderer->writeInstanceData(0, instanceData);
}

void RenderCube::draw(graphics::GameRenderer *renderer, std::vector<WorldObject> const &instances)
{
	// TODO: This should be making the draw call
}
