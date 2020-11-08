#pragma once

#include "TemportalEnginePCH.hpp"

#include "math/Vector.hpp"

#include <vulkan/vulkan.hpp>

NS_GRAPHICS
class CommandBuffer;
class RenderPass;
class FrameBuffer;
class Pipeline;
class Buffer;
class Image;

struct BufferRegionCopy
{
	uIndex srcOffset;
	uIndex dstOffset;
	uSize size;
};

class Command
{
	friend class CommandBuffer;

public:

	// For copying transfer buffers
	Command& copyBuffer(Buffer *src, Buffer *dest, ui64 size);
	Command& copyBuffer(Buffer *src, Buffer *dst, std::vector<BufferRegionCopy> const& copyOperations);

	Command& setPipelineImageBarrier(Image *image, vk::ImageLayout prevLayout, vk::ImageLayout nextLayout);
	Command& copyBufferToImage(Buffer *src, Image *dest);

	Command& beginRenderPass(RenderPass *pRenderPass, FrameBuffer *pFrameBuffer, math::Vector2UInt const resolution);
	Command& setViewport(vk::Viewport const &viewport);
	Command& bindPipeline(std::shared_ptr<Pipeline> pPipeline);
	Command& bindDescriptorSets(std::shared_ptr<Pipeline> pPipeline, std::vector<vk::DescriptorSet> sets);
	Command& bindVertexBuffers(ui32 bindingIndex, std::vector<Buffer*> const pBuffers);
	Command& bindIndexBuffer(ui64 offset, Buffer *pBuffer, vk::IndexType indexType);
	Command& draw(ui32 idxStartIndex, ui32 indexCount, ui32 indexShift, ui32 idxStartInstance, ui32 instanceCount);
	Command& endRenderPass();

	void end();

private:
	CommandBuffer *mpBuffer;
	void* mpVulkanBuffer;
	
	Command(CommandBuffer *pBuffer);

};

NS_END
