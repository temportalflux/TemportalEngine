#include "render/line/LineRenderer.hpp"

#include "asset/PipelineAsset.hpp"
#include "asset/Shader.hpp"
#include "graphics/Command.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/assetHelpers.hpp"

using namespace graphics;

LineRenderer::LineRenderer()
{
}

LineRenderer::~LineRenderer()
{
	destroy();
}

LineRenderer& LineRenderer::setPipeline(asset::TypedAssetPath<asset::Pipeline> const& path)
{
	if (!this->mpPipeline)
	{
		this->mpPipeline = std::make_shared<graphics::Pipeline>();
	}

	graphics::populatePipeline(path, this->mpPipeline.get(), nullptr);

	{
		ui8 slot = 0;
		this->mpPipeline->setBindings({ this->makeVertexBinding(slot) });
	}

	return *this;
}

void LineRenderer::createGraphicsBuffers(graphics::CommandPool* transientPool)
{
	this->mVertexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, graphics::MemoryUsage::eGPUOnly)
		.setSize(this->vertexBufferSize())
		.create();

	this->mIndexBuffer
		.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, graphics::MemoryUsage::eGPUOnly)
		.setSize(this->indexBufferSize())
		.create();

	// TODO: These can be done in one operation, and we don't need to wait for the graphics device to be done (nothing relies on this process except starting rendering)
	this->mVertexBuffer.writeBuffer(transientPool, 0, this->vertexBufferData(), this->vertexBufferSize());
	this->mIndexBuffer.writeBuffer(transientPool, 0, this->indexBufferData(), this->indexBufferSize());
}

// ~~~~~~~~~~ START: IPipelineRenderer ~~~~~~~~~~

void LineRenderer::setDevice(std::weak_ptr<graphics::GraphicsDevice> device)
{
	this->mpPipeline->setDevice(device);
	this->mVertexBuffer.setDevice(device);
	this->mIndexBuffer.setDevice(device);
}

void LineRenderer::setRenderPass(std::shared_ptr<graphics::RenderPass> renderPass)
{
	this->mpPipeline->setRenderPass(renderPass);
}

void LineRenderer::setDescriptorLayouts(std::unordered_map<std::string, graphics::DescriptorLayout const*> const& globalLayouts)
{
	this->mpPipeline->setDescriptorLayouts({ globalLayouts.find("camera")->second });
}

void LineRenderer::createPipeline(math::Vector2UInt const& resolution)
{
	this->mpPipeline->setResolution(resolution).create();
}

void LineRenderer::record(graphics::Command *command, uIndex idxFrame, TGetGlobalDescriptorSet getGlobalDescriptorSet)
{
	OPTICK_EVENT();
	command->bindDescriptorSets(this->mpPipeline, { getGlobalDescriptorSet("camera", idxFrame) });
	command->bindPipeline(this->mpPipeline);
	command->bindVertexBuffers(0, { &this->mVertexBuffer });
	command->bindIndexBuffer(0, &this->mIndexBuffer, vk::IndexType::eUint16);
	this->draw(command);
}

void LineRenderer::destroyRenderChain()
{
	this->mpPipeline->invalidate();
}

// ~~~~~~~~~~~~ END: IPipelineRenderer ~~~~~~~~~~

void LineRenderer::destroy()
{
	this->mVertexBuffer.destroy();
	this->mIndexBuffer.destroy();
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
