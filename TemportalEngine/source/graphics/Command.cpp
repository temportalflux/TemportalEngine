#include "graphics/Command.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/FrameBuffer.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/Buffer.hpp"
#include "graphics/Image.hpp"

using namespace graphics;

Command::Command(CommandBuffer *pBuffer)
	: mpBuffer(pBuffer)
{
}

Command& Command::clearColor(std::array<f32, 4U> color)
{
	this->mClearValues.push_back(vk::ClearValue().setColor(vk::ClearColorValue(color)));
	return *this;
}

Command& Command::clearDepth(f32 depth, ui32 stencil)
{
	this->mClearValues.push_back(vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue(depth, stencil)));
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

Command& Command::setPipelineImageBarrier(Image *image, vk::ImageLayout prevLayout, vk::ImageLayout nextLayout)
{
	auto& barrier = vk::ImageMemoryBarrier()
		.setOldLayout(prevLayout).setNewLayout(nextLayout)
		.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		.setImage(*reinterpret_cast<vk::Image*>(image->get()))
		.setSubresourceRange(
			vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0).setLevelCount(1)
			.setBaseArrayLayer(0).setLayerCount(1)
		)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstAccessMask(vk::AccessFlags());

	if (nextLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		if (image->hasStencilComponent())
		{
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}

	vk::PipelineStageFlags srcStage;
	vk::PipelineStageFlags dstStage;
	if (prevLayout == vk::ImageLayout::eUndefined && nextLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (prevLayout == vk::ImageLayout::eTransferDstOptimal && nextLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
		srcStage = vk::PipelineStageFlagBits::eTransfer;
		dstStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (prevLayout == vk::ImageLayout::eUndefined && nextLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		barrier.setDstAccessMask(
			vk::AccessFlagBits::eDepthStencilAttachmentRead
			| vk::AccessFlagBits::eDepthStencilAttachmentWrite
		);
		srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
		dstStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition");
	}
	this->mpBuffer->mInternal->pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, { barrier });
	return *this;
}

Command& Command::copyBufferToImage(Buffer *src, Image *dest)
{
	auto imgSize = dest->getSize();
	auto region = vk::BufferImageCopy()
		.setBufferOffset(0)
		.setBufferRowLength(0)
		.setBufferImageHeight(0)
		.setImageSubresource(
			vk::ImageSubresourceLayers()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setMipLevel(0)
			.setBaseArrayLayer(0).setLayerCount(1)
		)
		.setImageOffset(vk::Offset3D().setX(0).setY(0).setZ(0))
		.setImageExtent(
			vk::Extent3D()
			.setWidth(imgSize.x())
			.setHeight(imgSize.y())
			.setDepth(imgSize.z())
		);
	this->mpBuffer->mInternal->copyBufferToImage(
		*reinterpret_cast<vk::Buffer*>(src->get()),
		*reinterpret_cast<vk::Image*>(dest->get()),
		vk::ImageLayout::eTransferDstOptimal,
		{ region }
	);
	return *this;
}

Command& Command::beginRenderPass(RenderPass const *pRenderPass, FrameBuffer const *pFrameBuffer)
{
	this->mpBuffer->mInternal->beginRenderPass(
		vk::RenderPassBeginInfo()
		.setRenderPass(pRenderPass->mRenderPass.get())
		.setFramebuffer(pFrameBuffer->mInternal.get())
		.setClearValueCount((ui32)this->mClearValues.size())
		.setPClearValues(this->mClearValues.data())
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

Command& Command::bindDescriptorSet(Pipeline const *pPipeline, vk::DescriptorSet const *set)
{
	this->mpBuffer->mInternal->bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, pPipeline->mLayout.get(),
		0, 1, set, 0, nullptr
	);
	return *this;
}

Command& Command::bindVertexBuffers(ui32 bindingIndex, std::vector<Buffer*> const pBuffers)
{
	ui32 bufferCount = (ui32)pBuffers.size();
	auto buffers = std::vector<vk::Buffer>(bufferCount);
	auto offsets = std::vector<ui64>(bufferCount);
	for (ui32 i = 0; i < bufferCount; ++i)
	{
		buffers[i] = *reinterpret_cast<vk::Buffer*>(pBuffers[i]->get());
		offsets[i] = 0; // NOTE: should probably be passed in or stored in buffer wrapper
	}
	this->mpBuffer->mInternal->bindVertexBuffers(bindingIndex, { bufferCount, buffers.data() }, { bufferCount, offsets.data() });
	return *this;
}

Command& Command::bindIndexBuffer(ui64 offset, Buffer* const pBuffer, vk::IndexType indexType)
{
	this->mpBuffer->mInternal->bindIndexBuffer(*reinterpret_cast<vk::Buffer*>(pBuffer->get()), offset, indexType);
	return *this;
}

Command& Command::draw(ui32 indexCount, ui32 instanceCount)
{
	this->mpBuffer->mInternal->drawIndexed(
		indexCount, instanceCount,
		/*firstIndex*/ 0, /*vertexOffset*/ 0, /*firstInstace*/ 0
	);
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
