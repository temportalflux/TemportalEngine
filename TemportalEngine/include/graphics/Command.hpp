#pragma once

#include "TemportalEnginePCH.hpp"

#include "types/integer.h"
#include "types/real.h"

#include <array>
#include <vector>
#include <optional>
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
	
	Command(CommandBuffer *pBuffer);

};

NS_END
