#include "render/RenderBlocks.hpp"

#include "graphics/GameRenderer.hpp"
#include "graphics/Command.hpp"
#include "math/Vector.hpp"
#include "math/Matrix.hpp" 
#include "model/ModelCube.hpp"

#include "world/BlockInstanceMap.hpp"

RenderBlocks::RenderBlocks(std::weak_ptr<game::VoxelModelManager> registry)
{
	this->mpBlockRegistry = registry;
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
}

std::vector<graphics::AttributeBinding> RenderBlocks::getBindings(ui8 &slot) const
{
	auto bindings = ModelCube::bindings(slot);
	bindings.push_back(world::BlockInstanceMap::getBinding(slot));
	return bindings;
}

void RenderBlocks::init(graphics::GameRenderer *renderer)
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

		this->mIndexBufferUnitType = vk::IndexType::eUint16;
		this->mIndexCount = (ui32)model.indicies().size();
		this->mIndexBuffer.setDevice(device);
		this->mIndexBuffer.setSize(model.getIndexBufferSize());
		this->mIndexBuffer.create();
		this->mIndexBuffer.configureSlot(this->mpBufferMemory);

		this->mpBufferMemory->create();

		this->mVertexBuffer.bindMemory();
		this->mIndexBuffer.bindMemory();
	}
	
	// Write model data
	this->mVertexBuffer.writeBuffer(&renderer->getTransientPool(), 0, model.verticies());
	this->mIndexBuffer.writeBuffer(&renderer->getTransientPool(), 0, model.indicies());

	this->mpBlockRenderInstances = std::make_shared<world::BlockInstanceMap>();
	this->mpBlockRenderInstances->setDevice(device);
	this->mpBlockRenderInstances->constructInstanceBuffer(6, 16);
}

void RenderBlocks::record(graphics::Command *command)
{
	command->bindVertexBuffers(0, { &this->mVertexBuffer });
	command->bindIndexBuffer(0, &this->mIndexBuffer, this->mIndexBufferUnitType);

	auto const registry = this->mpBlockRegistry.lock();
	//game::VoxelModelManager::IterEntryItem iter;
	//for (iter = registry->begin(); iter != registry->end(); ++iter)
	{
		auto instanceData = this->mpBlockRenderInstances->getBlockInstanceData({});// iter->id);
		command->bindVertexBuffers(1, { instanceData.buffer });
		command->draw(0, this->mIndexCount, 0, instanceData.offset, instanceData.count);
	}
}

void RenderBlocks::invalidate()
{
	this->mVertexBuffer.destroy();
	this->mIndexBuffer.destroy();
	this->mpBufferMemory->destroy();
	this->mpBufferMemory.reset();
}

std::function<void(
	world::Coordinate const&, std::optional<BlockMetadata> const&, std::optional<BlockMetadata> const&
)> RenderBlocks::onBlockChangedListener()
{
	return this->mpBlockRenderInstances->onBlockChangedListener();
}

void RenderBlocks::writeInstanceBuffer(graphics::CommandPool* transientPool)
{
	this->mpBlockRenderInstances->writeInstanceBuffer(transientPool);
}
