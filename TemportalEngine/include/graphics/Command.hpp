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

class Command
{
	friend class CommandBuffer;

public:

	// TODO: pass engine vector instead of std array
	Command& clearColor(std::array<f32, 4U> color);
	Command& clearDepth(f32 depth, ui32 stencil);
	Command& setRenderArea(math::Vector2Int const offset, math::Vector2UInt const size);

	// For copying transfer buffers
	Command& copyBuffer(Buffer *src, Buffer *dest, ui64 size);

	Command& setPipelineImageBarrier(Image *image, vk::ImageLayout prevLayout, vk::ImageLayout nextLayout);
	Command& copyBufferToImage(Buffer *src, Image *dest);

	Command& beginRenderPass(RenderPass *pRenderPass, FrameBuffer *pFrameBuffer);
	Command& bindPipeline(Pipeline const *pPipeline);
	Command& bindDescriptorSet(Pipeline const *pPipeline, vk::DescriptorSet const *set);
	Command& bindVertexBuffers(ui32 bindingIndex, std::vector<Buffer*> const pBuffers);
	Command& bindIndexBuffer(ui64 offset, Buffer *pBuffer, vk::IndexType indexType);
	Command& draw(ui32 vertexCount, ui32 instanceCount = 1);
	Command& endRenderPass();

	void end();

private:
	CommandBuffer *mpBuffer;
	void* mpVulkanBuffer;
	std::vector<vk::ClearValue> mClearValues;
	math::Vector2Int mRenderAreaOffset;
	math::Vector2UInt mRenderAreaSize;
	
	Command(CommandBuffer *pBuffer);

};

NS_END
