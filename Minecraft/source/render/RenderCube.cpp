#include "render/RenderCube.hpp"

#include "graphics/GameRenderer.hpp"
#include "graphics/Command.hpp"
#include "model/ModelCube.hpp"

RenderCube::RenderCube()
{
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
	this->mInstanceBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
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

	auto device = renderer->getDevice();

	this->mpBufferMemory = std::make_shared<graphics::Memory>();
	this->mpBufferMemory->setDevice(device);
	this->mpBufferMemory->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	// Initialize Buffers
	{
		this->mVertexBuffer.setDevice(device);
		this->mVertexBuffer.setSize(model.getVertexBufferSize());
		this->mVertexBuffer.create();
		this->mVertexBuffer.configureSlot(this->mpBufferMemory);

		this->mInstanceCount = (ui32)instances.size();
		this->mInstanceBuffer.setDevice(device);
		this->mInstanceBuffer.setSize(this->mInstanceCount * sizeof(WorldObject::InstanceData));
		this->mInstanceBuffer.create();
		this->mInstanceBuffer.configureSlot(this->mpBufferMemory);

		this->mIndexBufferUnitType = vk::IndexType::eUint16;
		this->mIndexCount = (ui32)model.indicies().size();
		this->mIndexBuffer.setDevice(device);
		this->mIndexBuffer.setSize(model.getIndexBufferSize());
		this->mIndexBuffer.create();
		this->mIndexBuffer.configureSlot(this->mpBufferMemory);

		this->mpBufferMemory->create();

		this->mVertexBuffer.bindMemory();
		this->mInstanceBuffer.bindMemory();
		this->mIndexBuffer.bindMemory();
	}
	
	// Write model data
	this->mVertexBuffer.writeBuffer(&renderer->getTransientPool(), 0, model.verticies());
	this->mIndexBuffer.writeBuffer(&renderer->getTransientPool(), 0, model.indicies());

	// Write static instance data
	auto instanceData = std::vector<WorldObject::InstanceData>(instances.size());
	std::transform(
		std::begin(instances), std::end(instances),
		std::begin(instanceData),
		[](WorldObject inst) -> WorldObject::InstanceData { return { inst.getModelMatrix() }; }
	);
	this->mInstanceBuffer.writeBuffer(&renderer->getTransientPool(), 0, instanceData);

	this->instances = instances;
}

void RenderCube::draw(graphics::Command *command)
{
	command->bindVertexBuffers(0, { &this->mVertexBuffer });
	command->bindVertexBuffers(1, { &this->mInstanceBuffer });
	command->bindIndexBuffer(0, &this->mIndexBuffer, this->mIndexBufferUnitType);
	command->draw(0, this->mIndexCount, 0, 0, this->mInstanceCount);
}

void RenderCube::invalidate()
{
	this->mVertexBuffer.destroy();
	this->mInstanceBuffer.destroy();
	this->mIndexBuffer.destroy();
	this->mpBufferMemory->destroy();
	this->mpBufferMemory.reset();
}
