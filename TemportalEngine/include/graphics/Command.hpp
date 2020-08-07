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

	// For copying transfer buffers
	Command& copyBuffer(Buffer *src, Buffer *dest, ui64 size);

	Command& setPipelineImageBarrier(Image *image, vk::ImageLayout prevLayout, vk::ImageLayout nextLayout);
	Command& copyBufferToImage(Buffer *src, Image *dest);

	Command& beginRenderPass(RenderPass *pRenderPass, FrameBuffer *pFrameBuffer, math::Vector2UInt const resolution);
	Command& setViewport(vk::Viewport const &viewport);
	Command& bindPipeline(std::shared_ptr<Pipeline> pPipeline);
	Command& bindDescriptorSets(std::shared_ptr<Pipeline> pPipeline, vk::DescriptorSet const *set);
	Command& bindVertexBuffers(ui32 bindingIndex, std::vector<Buffer*> const pBuffers);
	Command& bindIndexBuffer(ui64 offset, Buffer *pBuffer, vk::IndexType indexType);
	Command& draw(ui32 vertexCount, ui32 instanceCount = 1);
	Command& endRenderPass();

	void end();

private:
	CommandBuffer *mpBuffer;
	void* mpVulkanBuffer;
	
	Command(CommandBuffer *pBuffer);

};

NS_END
