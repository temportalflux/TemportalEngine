#include "graphics/Command.hpp"

#include "graphics/CommandBuffer.hpp"
#include "graphics/Descriptor.hpp"
#include "graphics/RenderPass.hpp"
#include "graphics/FrameBuffer.hpp"
#include "graphics/Pipeline.hpp"
#include "graphics/Buffer.hpp"
#include "graphics/Image.hpp"
#include "graphics/VulkanApi.hpp"

using namespace graphics;

Command::Command(CommandBuffer *pBuffer)
	: mpBuffer(pBuffer)
{
	this->mpVulkanBuffer = this->mpBuffer->get();
}

vk::CommandBuffer* castBuf(void* ptr)
{
	return reinterpret_cast<vk::CommandBuffer*>(ptr);
}

Command& Command::copyBuffer(Buffer *src, Buffer *dest, ui64 size)
{
	OPTICK_EVENT();
	auto region = vk::BufferCopy().setSrcOffset(0).setDstOffset(0).setSize(size);
	castBuf(this->mpVulkanBuffer)->copyBuffer(
		extract<vk::Buffer>(src),
		extract<vk::Buffer>(dest),
		1, &region
	);
	return *this;
}

Command& Command::copyBuffer(Buffer *src, Buffer *dst, std::vector<BufferRegionCopy> const& copyOperations)
{
	OPTICK_EVENT();
	castBuf(this->mpVulkanBuffer)->copyBuffer(
		extract<vk::Buffer>(src),
		extract<vk::Buffer>(dst),
		(ui32)copyOperations.size(),
		(vk::BufferCopy*)copyOperations.data()
	);
	return *this;
}

Command& Command::setPipelineImageBarrier(Image *image, vk::ImageLayout prevLayout, vk::ImageLayout nextLayout)
{
	OPTICK_EVENT();
	auto& barrier = vk::ImageMemoryBarrier()
		.setOldLayout(prevLayout).setNewLayout(nextLayout)
		.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		.setImage(extract<vk::Image>(image))
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
	castBuf(this->mpVulkanBuffer)->pipelineBarrier(srcStage, dstStage, vk::DependencyFlags(), {}, {}, { barrier });
	return *this;
}

Command& Command::copyBufferToImage(Buffer *src, Image *dest)
{
	OPTICK_EVENT();
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
	castBuf(this->mpVulkanBuffer)->copyBufferToImage(
		extract<vk::Buffer>(src),
		extract<vk::Image>(dest),
		vk::ImageLayout::eTransferDstOptimal,
		{ region }
	);
	return *this;
}

Command& Command::beginRenderPass(RenderPass *pRenderPass, FrameBuffer *pFrameBuffer, math::Vector2UInt const resolution)
{
	OPTICK_EVENT();
	// TODO: RenderPass objects can be configured with multiple clear values so long as the clear values correspond to a specific FrameBuffer attachment
	auto clearValues = std::vector<vk::ClearValue>();
	if (pRenderPass->clearColor())
	{
		math::Vector4 clearColor = *pRenderPass->clearColor();
		clearValues.push_back(vk::ClearValue().setColor(vk::ClearColorValue(
			std::array<f32, 4>({ clearColor.x(), clearColor.y(), clearColor.z(), clearColor.w() })
		)));
	}
	if (pRenderPass->clearDepthStencil())
	{
		auto depthStencil = *pRenderPass->clearDepthStencil();
		clearValues.push_back(vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue(
			depthStencil.first, depthStencil.second
		)));
	}

	auto renderArea = pRenderPass->renderArea();
	renderArea.offset *= resolution.toFloat();
	renderArea.size *= resolution.toFloat();

	castBuf(this->mpVulkanBuffer)->beginRenderPass(
		vk::RenderPassBeginInfo()
		.setRenderPass(extract<vk::RenderPass>(pRenderPass))
		.setFramebuffer(extract<vk::Framebuffer>(pFrameBuffer))
		.setClearValueCount((ui32)clearValues.size())
		.setPClearValues(clearValues.data())
		.setRenderArea(
			vk::Rect2D()
			.setOffset({ (i32)renderArea.offset.x(), (i32)renderArea.offset.y() })
			.setExtent({ (ui32)renderArea.size.x(), (ui32)renderArea.size.y() })
		),
		vk::SubpassContents::eInline
	);
	return *this;
}

Command& Command::setViewport(vk::Viewport const &viewport)
{
	castBuf(this->mpVulkanBuffer)->setViewport(0, { viewport });
	return *this;
}

Command& Command::bindPipeline(std::shared_ptr<Pipeline> pPipeline)
{
	OPTICK_EVENT();
	castBuf(this->mpVulkanBuffer)->bindPipeline(vk::PipelineBindPoint::eGraphics, pPipeline->mPipeline.get());
	return *this;
}

Command& Command::bindDescriptorSets(std::shared_ptr<Pipeline> pPipeline, std::vector<vk::DescriptorSet> sets)
{
	OPTICK_EVENT();
	castBuf(this->mpVulkanBuffer)->bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics, pPipeline->mLayout.get(),
		0, (ui32)sets.size(), sets.data(), 0, nullptr
	);
	return *this;
}

Command& Command::bindDescriptorSets(std::shared_ptr<Pipeline> pPipeline, std::vector<graphics::DescriptorSet*> sets)
{
	auto vkSets = std::vector<vk::DescriptorSet>();
	for (auto const* pSet : sets)
	{
		vkSets.push_back(reinterpret_cast<VkDescriptorSet>(pSet->get()));
	}
	return bindDescriptorSets(pPipeline, vkSets);
}

Command& Command::bindVertexBuffers(ui32 bindingIndex, std::vector<Buffer*> const pBuffers)
{
	OPTICK_EVENT();
	ui32 bufferCount = (ui32)pBuffers.size();
	auto buffers = std::vector<vk::Buffer>(bufferCount);
	auto offsets = std::vector<ui64>(bufferCount);
	for (ui32 i = 0; i < bufferCount; ++i)
	{
		buffers[i] = extract<vk::Buffer>(pBuffers[i]);
		offsets[i] = 0; // NOTE: should probably be passed in or stored in buffer wrapper
	}
	castBuf(this->mpVulkanBuffer)->bindVertexBuffers(bindingIndex, { bufferCount, buffers.data() }, { bufferCount, offsets.data() });
	return *this;
}

Command& Command::bindIndexBuffer(ui64 offset, Buffer *pBuffer, vk::IndexType indexType)
{
	OPTICK_EVENT();
	castBuf(this->mpVulkanBuffer)->bindIndexBuffer(extract<vk::Buffer>(pBuffer), offset, indexType);
	return *this;
}

Command& Command::draw(ui32 idxStartIndex, ui32 indexCount, ui32 indexShift, ui32 idxStartInstance, ui32 instanceCount)
{
	OPTICK_EVENT();
	castBuf(this->mpVulkanBuffer)->drawIndexed(
		indexCount, instanceCount,
		idxStartIndex, indexShift,
		idxStartInstance
	);
	return *this;
}

Command& Command::endRenderPass()
{
	OPTICK_EVENT();
	castBuf(this->mpVulkanBuffer)->endRenderPass();
	return *this;
}

void Command::end()
{
	OPTICK_EVENT();
	this->mpBuffer->endCommand(this);
}
