#include "render/RenderCube.hpp"

#include "graphics/GameRenderer.hpp"
#include "graphics/Command.hpp"
#include "math/Vector.hpp"
#include "math/Matrix.hpp" 
#include "model/ModelCube.hpp"

struct CubeInstance
{
	math::Vector3 posInChunk;
	math::Matrix4x4 model;
};

RenderCube::RenderCube()
{
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
	this->mInstanceBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
}

std::vector<graphics::AttributeBinding> RenderCube::getBindings(ui8 &slot) const
{
	auto bindings = ModelCube::bindings(slot);
	bindings.push_back(
		// Data per object instance - this is only for objects which dont more, rotate, or scale
		graphics::AttributeBinding(graphics::AttributeBinding::Rate::eInstance)
		.setStructType<CubeInstance>()
		.addAttribute({ /*slot*/ slot++, /*vec3*/ (ui32)vk::Format::eR32G32B32Sfloat, offsetof(CubeInstance, posInChunk) })
		// mat4 using 4 slots
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(CubeInstance, model) + (0 * sizeof(glm::vec4)) })
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(CubeInstance, model) + (1 * sizeof(glm::vec4)) })
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(CubeInstance, model) + (2 * sizeof(glm::vec4)) })
		.addAttribute({ /*slot*/ slot++, /*mat4*/ (ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(CubeInstance, model) + (3 * sizeof(glm::vec4)) })
	);
	return bindings;
}

void RenderCube::init(graphics::GameRenderer *renderer, std::vector<world::Coordinate> const &instances)
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
		this->mInstanceBuffer.setSize(this->mInstanceCount * sizeof(CubeInstance));
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
	auto instanceData = std::vector<CubeInstance>(instances.size());
	std::transform(
		std::begin(instances), std::end(instances),
		std::begin(instanceData),
		[](world::Coordinate coordinate) -> CubeInstance { return {
			coordinate.chunk().toFloat(),
			math::createModelMatrix(coordinate.local().toFloat())
		}; }
	);
	this->mInstanceBuffer.writeBuffer(&renderer->getTransientPool(), 0, instanceData);
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
