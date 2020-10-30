#include "world/BlockInstanceMap.hpp"

#include "graphics/Command.hpp"
#include "graphics/Memory.hpp"

using namespace world;

graphics::AttributeBinding BlockInstanceMap::getBinding(ui8& slot)
{
	auto modelMatrix = (ui32)offsetof(RenderData, model);
	auto vec4size = (ui32)sizeof(math::Vector4);
	// Data per object instance - this is only for objects which dont more, rotate, or scale
	return graphics::AttributeBinding(graphics::AttributeBinding::Rate::eInstance)
		.setStructType<RenderData>()
		.addAttribute({ /*slot*/ slot++, /*vec3*/ (ui32)vk::Format::eR32G32B32Sfloat, offsetof(RenderData, posOfChunk) })
		// mat4 using 4 slots
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, modelMatrix + (0 * vec4size) })
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, modelMatrix + (1 * vec4size) })
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, modelMatrix + (2 * vec4size) })
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, modelMatrix + (3 * vec4size) });
}

BlockInstanceMap::BlockInstanceMap()
{
	this->mpInstanceMemory = std::make_shared<graphics::Memory>();
	this->mInstanceBuffer.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
}

BlockInstanceMap::~BlockInstanceMap()
{
	this->mInstanceBuffer.destroy();
	this->mpInstanceMemory.reset();
}

void BlockInstanceMap::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpInstanceMemory->setDevice(device);
	this->mInstanceBuffer.setDevice(device);
}

void BlockInstanceMap::constructInstanceBuffer(ui8 chunkRenderDistance, ui8 chunkSideLength)
{
	auto const instanceBufferSize = this->getInstanceBufferSize(chunkRenderDistance, chunkSideLength);

	// Deconstruct the previous buffer
	this->invalidate();

	this->mpInstanceMemory->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);

	this->mInstanceBuffer.setSize(instanceBufferSize);
	this->mInstanceBuffer.create();
	this->mInstanceBuffer.configureSlot(this->mpInstanceMemory);

	this->mpInstanceMemory->create();

	this->mInstanceBuffer.bindMemory();
}

ui64 BlockInstanceMap::getInstanceBufferSize(ui8 chunkRenderDistance, ui8 chunkSideLength) const
{
	// Computes (2c+1)^3 by filling all values of a 3-dimensional vector with `2c+1` and multiplying the dimensions together
	// If CRD=5, then count=1331
	// If CRD=6, then count=2197
	// If CRD=11, then count=12167
	ui32 const chunkCount = math::Vector<ui32, 3>(2 * chunkRenderDistance + 1).powDim();
	// the amount of blocks in a cube whose side length is CSL
	// If CSL=16, then blocksPerChunk=4096
	ui32 const blocksPerChunk = math::Vector<ui32, 3>(chunkSideLength).powDim();
	// CRD=5  & CSL=16 ->  1331*4096 ->  5,451,776
	// CRD=6  & CSL=16 ->  2197*4096 ->  8,998,912
	// CRD=11 & CSL=16 -> 12167*4096 -> 49,836,032
	ui64 const totalBlockCount = ui64(chunkCount) * ui64(blocksPerChunk);
	// RenderData has 5 vec4, which is 80 bytes (float is 4 bytes, 4 floats is 16 bytes per vec4, 5 vec4s is 16*5=80)
	// CRD=5  & CSL=16 ->  5,451,776 * 80 ->   436,142,080
	// CRD=6  & CSL=16 ->  8,998,912 * 80 ->   719,912,960 (CRD=7 puts this over 1GB)
	// CRD=11 & CSL=16 -> 49,836,032 * 80 -> 3,986,882,560 (CRD=12 puts this over 4GB)
	return totalBlockCount * sizeof(RenderData);
}

void BlockInstanceMap::invalidate()
{
	this->mInstanceBuffer.invalidate();
	this->mpInstanceMemory->invalidate();
}

BlockInstanceMap::TOnBlockChanged BlockInstanceMap::onBlockChangedListener()
{
	return std::bind(
		&BlockInstanceMap::updateCoordinate, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3
	);
}

void BlockInstanceMap::updateCoordinate(
	world::Coordinate const& global,
	std::optional<BlockMetadata> const& prev,
	std::optional<BlockMetadata> const& next
)
{
}

void BlockInstanceMap::writeInstanceBuffer(graphics::CommandPool* transientPool)
{
	this->mInstanceCount = 0;
	auto coords = std::vector<world::Coordinate>();
	for (i32 x = -3; x <= 3; ++x)
		for (i32 y = -3; y <= 3; ++y)
		{
			coords.push_back(world::Coordinate({ 0, 0, 0 }, { x, y, 0 }));
			this->mInstanceCount++;
		}

	auto instanceData = std::vector<RenderData>();
	std::transform(
		std::begin(coords), std::end(coords),
		std::back_inserter(instanceData),
		[](world::Coordinate coordinate) -> RenderData { return {
			coordinate.chunk().toFloat(),
			math::createModelMatrix(coordinate.local().toFloat())
		}; }
	);

	this->mInstanceBuffer.writeBuffer(transientPool, 0, instanceData);
}

BlockInstanceMap::InstanceData BlockInstanceMap::getBlockInstanceData(game::BlockId const &id)
{
	OPTICK_EVENT();
	return {
		&this->mInstanceBuffer,
		/*idxStart*/ 0, /*count*/ this->mInstanceCount
	};
}

