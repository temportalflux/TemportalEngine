#include "graphics/Command.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/FrameBuffer.hpp"
#include "graphics/Pipeline.hpp"

using namespace graphics;

Command::Command(CommandBuffer *pBuffer)
	: mpBuffer(pBuffer)
	, mClearValue(std::nullopt)
{
}

Command& Command::clear(std::array<f32, 4U> color)
{
	this->mClearValue = vk::ClearValue().setColor(vk::ClearColorValue(color));
	return *this;
}

Command& Command::beginRenderPass(RenderPass const *pRenderPass, FrameBuffer const *pFrameBuffer)
{
	this->mpBuffer->mInternal->beginRenderPass(
		vk::RenderPassBeginInfo()
		.setRenderPass(pRenderPass->mRenderPass.get())
		.setFramebuffer(pFrameBuffer->mInternal.get())
		.setClearValueCount(this->mClearValue.has_value() ? 1 : 0)
		.setPClearValues(this->mClearValue.has_value() ? &this->mClearValue.value() : nullptr)
		.setRenderArea(vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(pRenderPass->mResolution)
		),
		vk::SubpassContents::eInline
	);
	return *this;
}

Command& Command::bindPipeline(Pipeline const *pPipeline)
{
	this->mpBuffer->mInternal->bindPipeline(vk::PipelineBindPoint::eGraphics, pPipeline->mPipeline.get());
	return *this;
}

Command& Command::draw()
{
	// TODO: This is yet unfinished
	this->mpBuffer->mInternal->draw(3, 1, 0, 0);
	return *this;
}

Command& Command::endRenderPass()
{
	this->mpBuffer->mInternal->endRenderPass();
	return *this;
}

void Command::end()
{
	this->mpBuffer->endCommand(this);
}
