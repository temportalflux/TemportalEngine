#include "render/RenderCube.hpp"

#include "graphics/GameRenderer.hpp"
#include "graphics/Command.hpp"
#include "model/ModelCube.hpp"

RenderCube::RenderCube()
{
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
		.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
	this->mInstanceBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
		.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
		.setMemoryRequirements(vk::MemoryPropertyFlagBits::eDeviceLocal);
}

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

	// Initialize Buffers
	{
		this->mVertexBuffer.setSize(model.getVertexBufferSize());
		renderer->initializeBuffer(this->mVertexBuffer);

		this->mInstanceCount = (ui32)instances.size();
		this->mInstanceBuffer.setSize(this->mInstanceCount * sizeof(WorldObject::InstanceData));
		renderer->initializeBuffer(this->mInstanceBuffer);

		this->mIndexBuffer.setSize(model.getIndexBufferSize());
		this->mIndexBufferUnitType = vk::IndexType::eUint16;
		this->mIndexCount = (ui32)model.indicies().size();
		renderer->initializeBuffer(this->mIndexBuffer);
	}
	
	// Write model data
	renderer->writeBufferData(this->mVertexBuffer, 0, model.verticies());
	renderer->writeBufferData(this->mIndexBuffer, 0, model.indicies());

	// Write static instance data
	auto instanceData = std::vector<WorldObject::InstanceData>(instances.size());
	std::transform(
		std::begin(instances), std::end(instances),
		std::begin(instanceData),
		[](WorldObject inst) -> WorldObject::InstanceData { return { inst.getModelMatrix() }; }
	);
	renderer->writeBufferData(this->mInstanceBuffer, 0, instanceData);

	this->instances = instances;
}

void RenderCube::draw(graphics::Command *command)
{
	command->bindVertexBuffers(0, { &this->mVertexBuffer });
	command->bindVertexBuffers(1, { &this->mInstanceBuffer });
	command->bindIndexBuffer(0, &this->mIndexBuffer, this->mIndexBufferUnitType);
	command->draw(this->mIndexCount, this->mInstanceCount);
}

void RenderCube::invalidate()
{
	this->mVertexBuffer.destroy();
	this->mInstanceBuffer.destroy();
	this->mIndexBuffer.destroy();
}
