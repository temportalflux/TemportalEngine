#include "graphics/Command.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/FrameBuffer.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/Buffer.hpp"

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

Command& Command::bindVertexBuffers(std::vector<Buffer*> const pBuffers)
{
	ui32 bufferCount = (ui32)pBuffers.size();
	auto buffers = std::vector<vk::Buffer>(bufferCount);
	auto offsets = std::vector<ui64>(bufferCount);
	for (ui32 i = 0; i < bufferCount; ++i)
	{
		buffers[i] = *reinterpret_cast<vk::Buffer*>(pBuffers[i]->get());
		offsets[i] = 0; // NOTE: should probably be passed in or stored in buffer wrapper
	}
	this->mpBuffer->mInternal->bindVertexBuffers(0, { bufferCount, buffers.data() }, { bufferCount, offsets.data() });
	return *this;
}

Command& Command::bindIndexBuffer(ui64 offset, Buffer* const pBuffer, vk::IndexType indexType)
{
	this->mpBuffer->mInternal->bindIndexBuffer(*reinterpret_cast<vk::Buffer*>(pBuffer->get()), offset, indexType);
	return *this;
}

Command& Command::draw(ui32 indexCount)
{
	this->mpBuffer->mInternal->drawIndexed(indexCount, 1, 0, 0, 0);
	return *this;
}

Command& Command::endRenderPass()
{
	this->mpBuffer->mInternal->endRenderPass();
	return *this;
}

Command& Command::copyBuffer(Buffer *src, Buffer *dest, ui64 size)
{
	auto region = vk::BufferCopy().setSrcOffset(0).setDstOffset(0).setSize(size);
	this->mpBuffer->mInternal->copyBuffer(
		*reinterpret_cast<vk::Buffer*>(src->get()),
		*reinterpret_cast<vk::Buffer*>(dest->get()),
		1, &region
	);
	return *this;
}

void Command::end()
{
	this->mpBuffer->endCommand(this);
}
