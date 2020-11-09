#include "render/line/LineRenderer.hpp"

#include "asset/PipelineAsset.hpp"
#include "asset/Shader.hpp"
#include "graphics/Command.hpp"
#include "graphics/Memory.hpp"
#include "graphics/Pipeline.hpp"

using namespace graphics;

LineRenderer::LineRenderer(std::weak_ptr<graphics::DescriptorPool> pDescriptorPool)
{
	this->mpDescriptorPool = pDescriptorPool;
	this->mpMemoryGraphicsBuffers = std::make_shared<graphics::Memory>();
	this->mpMemoryGraphicsBuffers->setFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);
}

LineRenderer::~LineRenderer()
{
	destroyBuffers();
}

LineRenderer& LineRenderer::setPipeline(std::shared_ptr<asset::Pipeline> asset)
{
	if (!this->mpPipeline)
	{
		this->mpPipeline = std::make_shared<graphics::Pipeline>();
	}

	this->mpPipeline->addViewArea(asset->getViewport(), asset->getScissor());
	this->mpPipeline->setBlendMode(asset->getBlendMode());
	this->mpPipeline->setFrontFace(asset->getFrontFace());
	this->mpPipeline->setTopology(asset->getTopology());
	this->mpPipeline->setLineWidth(asset->getLineWidth());

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

ui32 LineRenderer::addLineSegment(LineSegment const& segment)
{
	this->mIndicies.push_back(this->pushVertex({ segment.pos1, segment.color }));
	this->mIndicies.push_back(this->pushVertex({ segment.pos2, segment.color }));
	return 2;
}

ui16 LineRenderer::pushVertex(LineVertex vertex)
{
	auto i = (ui16)this->mVerticies.size();
	this->mVerticies.push_back(vertex);
	return i;
}

ui32 LineRenderer::indexCount() const
{
	return (ui32)this->mIndicies.size();
}

void LineRenderer::createGraphicsBuffers(graphics::CommandPool* transientPool)
{
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
		.setSize(this->mVerticies.size() * sizeof(LineVertex))
		.create();
	this->mVertexBuffer.configureSlot(this->mpMemoryGraphicsBuffers);

	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
		.setSize(this->indexCount() * sizeof(ui16))
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

void LineRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpPipeline->setDevice(device);
	this->mpMemoryGraphicsBuffers->setDevice(device);
	this->mVertexBuffer.setDevice(device);
	this->mIndexBuffer.setDevice(device);
}

void LineRenderer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mpPipeline->setRenderPass(renderPass);
}

void LineRenderer::setFrameCount(uSize frameCount)
{
	this->mDescriptorGroups[0].setAmount((ui32)frameCount); // camera uniform needs per-frame
}

void LineRenderer::createDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	for (auto& descriptorGroup : this->mDescriptorGroups)
	{
		descriptorGroup.create(device, this->mpDescriptorPool.lock().get());
	}
}

void LineRenderer::attachDescriptors(
	std::unordered_map<std::string, std::vector<graphics::Buffer*>> &mutableUniforms
)
{
	this->mDescriptorGroups[0].attachToBinding("localCamera", mutableUniforms["localCamera"]);
}

void LineRenderer::writeDescriptors(std::shared_ptr<graphics::GraphicsDevice> device)
{
	for (auto& descriptorGroup : this->mDescriptorGroups)
	{
		descriptorGroup.writeAttachments(device);
	}
}

void LineRenderer::createPipeline(math::Vector2UInt const& resolution)
{
	this->mpPipeline
		->setDescriptors(&this->mDescriptorGroups)
		.setResolution(resolution)
		.create();
}

void LineRenderer::record(graphics::Command *command, uIndex idxFrame)
{
	OPTICK_EVENT();
	command->bindDescriptorSets(this->mpPipeline, { this->mDescriptorGroups[0].getDescriptorSet(idxFrame) });
	command->bindPipeline(this->mpPipeline);
	command->bindVertexBuffers(0, { &this->mVertexBuffer });
	command->bindIndexBuffer(0, &this->mIndexBuffer, vk::IndexType::eUint16);
	this->draw(command);
}

void LineRenderer::destroyRenderChain()
{
	this->mpPipeline->invalidate();
	for (auto& descriptorGroup : this->mDescriptorGroups)
	{
		descriptorGroup.invalidate();
	}
}

// ~~~~~~~~~~~~ END: IPipelineRenderer ~~~~~~~~~~

void LineRenderer::destroyBuffers()
{
	this->mVertexBuffer.destroy();
	this->mIndexBuffer.destroy();
	this->mpMemoryGraphicsBuffers.reset();
}

void LineRenderer::draw(graphics::Command *command)
{
	OPTICK_GPU_EVENT("DrawLines");
	command->draw(
		0, this->indexCount(),
		0, // index shift
		0, 1 // only a single instance, no instance buffer
	);
}
