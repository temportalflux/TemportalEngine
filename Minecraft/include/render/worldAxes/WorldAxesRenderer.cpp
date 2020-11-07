#include "render/worldAxes/WorldAxesRenderer.hpp"

#include "asset/PipelineAsset.hpp"
#include "asset/Shader.hpp"
#include "graphics/Command.hpp"
#include "graphics/Memory.hpp"
#include "graphics/Pipeline.hpp"

using namespace graphics;

WorldAxesRenderer::WorldAxesRenderer(std::weak_ptr<graphics::DescriptorPool> pDescriptorPool)
{
	this->mpDescriptorPool = pDescriptorPool;
	this->mpMemoryGraphicsBuffers = std::make_shared<graphics::Memory>();
	this->mpMemoryGraphicsBuffers->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);
}

WorldAxesRenderer::~WorldAxesRenderer()
{
	destroyBuffers();
}

WorldAxesRenderer& WorldAxesRenderer::setPipeline(std::shared_ptr<asset::Pipeline> asset)
{
	if (!this->mpPipeline)
	{
		this->mpPipeline = std::make_shared<graphics::Pipeline>();
	}

	this->mpPipeline->addViewArea(asset->getViewport(), asset->getScissor());
	this->mpPipeline->setBlendMode(asset->getBlendMode());
	this->mpPipeline->setFrontFace(asset->getFrontFace());
	this->mpPipeline->setTopology(asset->getTopology());

	// Perform a synchronous load on each shader to create the shader modules
	this->mpPipeline->addShader(asset->getVertexShader().load(asset::EAssetSerialization::Binary)->makeModule());
	this->mpPipeline->addShader(asset->getFragmentShader().load(asset::EAssetSerialization::Binary)->makeModule());

	{
		ui8 slot = 0;
		this->mpPipeline->setBindings({
			graphics::AttributeBinding(graphics::AttributeBinding::Rate::eVertex)
			.setStructType<LineVertex>()
			.addAttribute({ slot++, /*vec3*/(ui32)vk::Format::eR32G32B32Sfloat, offsetof(LineVertex, position) })
			.addAttribute({ slot++, /*vec4*/(ui32)vk::Format::eR32G32B32A32Sfloat, offsetof(LineVertex, color) })
		});
	}

	// Create descriptor groups for the pipeline
	for (auto const& assetDescGroup : asset->getDescriptorGroups())
	{
		auto const& descriptors = assetDescGroup.descriptors;
		auto descriptorGroup = graphics::DescriptorGroup();
		descriptorGroup.setBindingCount(descriptors.size());
		for (uIndex i = 0; i < descriptors.size(); ++i)
		{
			descriptorGroup.addBinding(descriptors[i].id, i, descriptors[i].type, descriptors[i].stage);
		}
		this->mDescriptorGroups.push_back(std::move(descriptorGroup));
	}

	return *this;
}

void WorldAxesRenderer::addLineSegment(math::Vector3Padded pos1, math::Vector3Padded pos2, math::Vector4 color)
{
	this->mIndicies.push_back(this->pushVertex({ pos1, color }));
	this->mIndicies.push_back(this->pushVertex({ pos2, color }));
}

ui16 WorldAxesRenderer::pushVertex(LineVertex vertex)
{
	auto i = (ui16)this->mVerticies.size();
	this->mVerticies.push_back(vertex);
	return i;
}

void WorldAxesRenderer::createGraphicsBuffers(graphics::CommandPool* transientPool)
{
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
		.setSize(this->mVerticies.size() * sizeof(LineVertex))
		.create();
	this->mVertexBuffer.configureSlot(this->mpMemoryGraphicsBuffers);

	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
		.setSize(this->mIndicies.size() * sizeof(ui16))
		.create();
	this->mIndexBuffer.configureSlot(this->mpMemoryGraphicsBuffers);

	this->mpMemoryGraphicsBuffers->create();

	this->mVertexBuffer.bindMemory();
	this->mIndexBuffer.bindMemory();

	// TODO: These can be done in one operation, and we don't need to wait for the graphics device to be done (nothing relies on this process except starting rendering)
	this->mVertexBuffer.writeBuffer(transientPool, 0, this->mVerticies);
	this->mIndexBuffer.writeBuffer(transientPool, 0, this->mIndicies);
}

// ~~~~~~~~~~ START: IPipelineRenderer ~~~~~~~~~~

void WorldAxesRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpPipeline->setDevice(device);
	this->mpMemoryGraphicsBuffers->setDevice(device);
	this->mVertexBuffer.setDevice(device);
	this->mIndexBuffer.setDevice(device);
}

void WorldAxesRenderer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mpPipeline->setRenderPass(renderPass);
}

void WorldAxesRenderer::setFrameCount(uSize frameCount)
{
	this->mDescriptorGroups[0].setAmount((ui32)frameCount); // camera uniform needs per-frame
}

void WorldAxesRenderer::createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	for (auto& descriptorGroup : this->mDescriptorGroups)
	{
		descriptorGroup.create(device, this->mpDescriptorPool.lock().get());
	}
}

void WorldAxesRenderer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
	this->mDescriptorGroups[0].attachToBinding("localCamera", mutableUniforms["localCamera"]);
}

void WorldAxesRenderer::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	for (auto& descriptorGroup : this->mDescriptorGroups)
	{
		descriptorGroup.writeAttachments(device);
	}
}

void WorldAxesRenderer::createPipeline(math::Vector2UInt const& resolution)
{
	this->mpPipeline
		->setDescriptors(&this->mDescriptorGroups)
		.setResolution(resolution)
		.create();
}

void WorldAxesRenderer::record(graphics::Command *command, uIndex idxFrame)
{
	OPTICK_EVENT();

	OPTICK_GPU_EVENT("DrawWorldAxes");
	command->bindDescriptorSets(this->mpPipeline, { this->mDescriptorGroups[0].getDescriptorSet(idxFrame) });
	command->bindPipeline(this->mpPipeline);
	command->bindVertexBuffers(0, { &this->mVertexBuffer });
	command->bindIndexBuffer(0, &this->mIndexBuffer, vk::IndexType::eUint16);
	command->draw(
		0, (ui32)this->mIndicies.size(),
		0, // index shift
		0, 1 // only a single instance, no instance buffer
	);

}

void WorldAxesRenderer::destroyRenderChain()
{
	this->mpPipeline->invalidate();
	for (auto& descriptorGroup : this->mDescriptorGroups)
	{
		descriptorGroup.invalidate();
	}
	this->mDescriptorGroups.clear();
}

// ~~~~~~~~~~~~ END: IPipelineRenderer ~~~~~~~~~~

void WorldAxesRenderer::destroyBuffers()
{
	this->mVertexBuffer.destroy();
	this->mIndexBuffer.destroy();
	this->mpMemoryGraphicsBuffers.reset();
}
